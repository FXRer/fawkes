
/***************************************************************************
 *  client.cpp - Fawkes network client
 *
 *  Created: Tue Nov 21 18:44:58 2006
 *  Copyright  2006  Tim Niemueller [www.niemueller.de]
 *
 *  $Id$
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <netcomm/fawkes/client.h>
#include <netcomm/fawkes/client_handler.h>
#include <netcomm/fawkes/message_queue.h>
#include <netcomm/fawkes/transceiver.h>
#include <netcomm/socket/stream.h>
#include <netcomm/utils/exceptions.h>

#include <core/threading/thread.h>
#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>
#include <core/threading/wait_condition.h>
#include <core/exceptions/system.h>

#include <list>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

namespace fawkes {

/** @class HandlerAlreadyRegisteredException netcomm/fawkes/client.h
 * Client handler has already been registered.
 * Only a single client handler can be registered per component. If you try
 * to register a handler where there is already a handler this exception
 * is thrown.
 * @ingroup NetComm
 */

/** Costructor. */
HandlerAlreadyRegisteredException::HandlerAlreadyRegisteredException()
  : Exception("A handler for this component has already been registered")
{
}


/** Fawkes network client send thread.
 * Spawned by the FawkesNetworkClient to handle outgoing traffic.
 *
 * @ingroup NetComm
 * @author Tim Niemueller
 */
class FawkesNetworkClientSendThread : public Thread
{
 public:

  /** Constructor.
   * @param s client stream socket
   * @param parent parent FawkesNetworkClient instance
   */
  FawkesNetworkClientSendThread(StreamSocket *s, FawkesNetworkClient *parent)
    : Thread("FawkesNetworkClientSendThread", Thread::OPMODE_WAITFORWAKEUP)
  {
    _s = s;
    _parent = parent;
    _outbound_msgq = new FawkesNetworkMessageQueue();
  }

  /** Destructor. */
  ~FawkesNetworkClientSendThread()
  {
    while ( ! _outbound_msgq->empty() ) {
      FawkesNetworkMessage *m = _outbound_msgq->front();
      m->unref();
      _outbound_msgq->pop();
    }
    delete _outbound_msgq;
  }

  virtual void once()
  {
    _parent->set_send_slave_alive();
  }

  virtual void loop()
  {
    if ( ! _parent->connected() )  return;

    if ( ! _outbound_msgq->empty() ) {
      try {
	FawkesNetworkTransceiver::send(_s, _outbound_msgq);
      } catch (ConnectionDiedException &e) {
	_parent->connection_died();
	exit();
      }
    }
  }

  /** Force sending of messages.
   * All messages are sent out immediately, if loop is not running already anyway.
   */
  void force_send()
  {
    if ( loop_mutex->try_lock() ) {
      loop();
      loop_mutex->unlock();
    }
  }

  /** Enqueue message to send.
   * @param message message to send
   */
  void enqueue(FawkesNetworkMessage *message)
  {
    message->ref();
    _outbound_msgq->push_locked(message);
    wakeup();
  }

 private:
  StreamSocket *_s;
  FawkesNetworkClient *_parent;
  FawkesNetworkMessageQueue *_outbound_msgq;
};


/**  Fawkes network client receive thread.
 * Spawned by the FawkesNetworkClient to handle incoming traffic.
 *
 * @ingroup NetComm
 * @author Tim Niemueller
 */
class FawkesNetworkClientRecvThread : public Thread
{
 public:
  /** Constructor.
   * @param s client stream socket
   * @param parent parent FawkesNetworkClient instance
   * @param recv_mutex receive mutex, locked while messages are received
   */
  FawkesNetworkClientRecvThread(StreamSocket *s, FawkesNetworkClient *parent,
				Mutex *recv_mutex)
    : Thread("FawkesNetworkClientRecvThread")
  {
    __s = s;
    __parent = parent;
    __inbound_msgq = new FawkesNetworkMessageQueue();
    __recv_mutex = recv_mutex;
  }

  /** Destructor. */
  ~FawkesNetworkClientRecvThread()
  {
    while ( ! __inbound_msgq->empty() ) {
      FawkesNetworkMessage *m = __inbound_msgq->front();
      m->unref();
      __inbound_msgq->pop();
    }
    delete __inbound_msgq;
  }

  /** Receive and process messages. */
  void recv()
  {
    std::list<unsigned int> wakeup_list;

    try {
      FawkesNetworkTransceiver::recv(__s, __inbound_msgq);

      MutexLocker lock(__recv_mutex);

      __inbound_msgq->lock();
      while ( ! __inbound_msgq->empty() ) {
	FawkesNetworkMessage *m = __inbound_msgq->front();
	wakeup_list.push_back(m->cid());
	__parent->dispatch_message(m);
	m->unref();
	__inbound_msgq->pop();
      }
      __inbound_msgq->unlock();

      lock.unlock();
    
      wakeup_list.sort();
      wakeup_list.unique();
      for (std::list<unsigned int>::iterator i = wakeup_list.begin(); i != wakeup_list.end(); ++i) {
	__parent->wake_handlers(*i);
      }
    } catch (ConnectionDiedException &e) {
      throw;
    }
  }

  virtual void once()
  {
    __parent->set_recv_slave_alive();
  }

  virtual void loop()
  {
    // just return if not connected
    if (! __s ) return;

    short p = 0;
    try {
      p = __s->poll();
    } catch (InterruptedException &e) {
      return;
    }

    if ( (p & Socket::POLL_ERR) ||
	 (p & Socket::POLL_HUP) ||
	 (p & Socket::POLL_RDHUP)) {
      __parent->connection_died();
      exit();
    } else if ( p & Socket::POLL_IN ) {
      // Data can be read
      try {
	recv();
      } catch (ConnectionDiedException &e) {
	__parent->connection_died();
	exit();
      }
    }
  }

 private:
  StreamSocket *__s;
  FawkesNetworkClient *__parent;
  FawkesNetworkMessageQueue *  __inbound_msgq;
  Mutex *__recv_mutex;
};


/** @class FawkesNetworkClient netcomm/fawkes/client.h
 * Simple Fawkes network client. Allows access to a remote instance via the
 * network. Encapsulates all needed interaction with the network.
 *
 * @ingroup NetComm
 * @author Tim Niemueller
 */

/** Constructor.
 * @param hostname remote host to connect to.
 * @param port port to connect to.
 * @param ip optional: use ip to connect, and hostname for cosmetic purposes
 */
FawkesNetworkClient::FawkesNetworkClient(const char *hostname, unsigned short int port, const char *ip)
{
  __hostname = strdup(hostname);
  __ip = ip ? strdup(ip) : NULL;
  __port     = port;

  s = NULL;
  __send_slave = NULL;
  __recv_slave = NULL;

  connection_died_recently = false;
  __send_slave_alive = false;
  __recv_slave_alive = false;

  slave_status_mutex = new Mutex();

  _id     = 0;
  _has_id = false;

  __recv_mutex          = new Mutex();
  __connest_mutex       = new Mutex();
  __connest_waitcond    = new WaitCondition(__connest_mutex);
  __connest             = false;
  __connest_interrupted = false;
}


/** Constructor.
 * Note, you cannot call the connect() without parameters the first time you
 * establish an connection when using this ctor!
 */
FawkesNetworkClient::FawkesNetworkClient()
{
  __hostname = NULL;
  __ip = NULL;
  __port     = 0;

  s = NULL;
  __send_slave = NULL;
  __recv_slave = NULL;

  connection_died_recently = false;
  __send_slave_alive = false;
  __recv_slave_alive = false;

  slave_status_mutex = new Mutex();

  _id     = 0;
  _has_id = false;

  __recv_mutex          = new Mutex();
  __connest_mutex       = new Mutex();
  __connest_waitcond    = new WaitCondition(__connest_mutex);
  __connest             = false;
  __connest_interrupted = false;
}


/** Constructor.
 * @param id id of the client.
 * @param hostname remote host to connect to.
 * @param port port to connect to.
 * @param ip optional: use ip to connect, and hostname for cosmetic purposes
 */
FawkesNetworkClient::FawkesNetworkClient(unsigned int id, const char *hostname,
                                         unsigned short int port, const char *ip)
{
  __hostname = strdup(hostname);
  __ip = ip ? strdup(ip) : NULL;
  __port     = port;

  s = NULL;
  __send_slave = NULL;
  __recv_slave = NULL;

  connection_died_recently = false;
  __send_slave_alive = false;
  __recv_slave_alive = false;

  slave_status_mutex = new Mutex();

  _id     = id;
  _has_id = true;

  __recv_mutex          = new Mutex();
  __connest_mutex       = new Mutex();
  __connest_waitcond    = new WaitCondition(__connest_mutex);
  __connest             = false;
  __connest_interrupted = false;
}


/** Destructor. */
FawkesNetworkClient::~FawkesNetworkClient()
{
  disconnect();

  for (std::map<unsigned int, WaitCondition *>::iterator i =  waitconds.begin(); i != waitconds.end(); ++i ) {
    delete (*i).second;
  }
  waitconds.clear();
  delete s;
  if (__hostname) free(__hostname);
  if (__ip) free(__ip);
  delete slave_status_mutex;

  delete __connest_waitcond;
  delete __connest_mutex;
  delete __recv_mutex;
}


/** Connect to remote.
 * @exception SocketException thrown by Socket::connect()
 * @exception NullPointerException thrown if hostname has not been set
 */
void
FawkesNetworkClient::connect()
{
  if ( __hostname == NULL && __ip == NULL) {
    throw NullPointerException("Hostname not set. Cannot connect.");
  }

  if ( s != NULL ) {
    disconnect();
  }


  connection_died_recently = false;

  try {
    s = new StreamSocket();
    s->connect(__ip ? __ip : __hostname, __port);
    __send_slave = new FawkesNetworkClientSendThread(s, this);
    __send_slave->start();
    __recv_slave = new FawkesNetworkClientRecvThread(s, this, __recv_mutex);
    __recv_slave->start();
  } catch (SocketException &e) {
    connection_died_recently = true;
    if ( __send_slave ) {
      __send_slave->cancel();
      __send_slave->join();
      delete __send_slave;
      __send_slave = NULL;
    }
    if ( __recv_slave ) {
      __recv_slave->cancel();
      __recv_slave->join();
      delete __recv_slave;
      __recv_slave = NULL;
    }
    __send_slave_alive = false;
    __recv_slave_alive = false;
    delete s;
    s = NULL;
    throw;
  }

  __connest_mutex->lock();
  while ( ! __connest && ! __connest_interrupted ) {
    __connest_waitcond->wait();
  }
  bool interrupted = __connest_interrupted;
  __connest_interrupted = false;
  __connest_mutex->unlock();
  if ( interrupted ) {
    throw InterruptedException("FawkesNetworkClient::connect()");
  }

  notify_of_connection_established();
}


/** Connect to new host and port.
 * @param hostname new hostname to connect to
 * @param port new port to connect to
 * @see connect() Look there for more documentation and notes about possible
 * exceptions.
 */
void
FawkesNetworkClient::connect(const char *hostname, unsigned short int port)
{
  connect(hostname, NULL, port);
}

/** Connect to new ip and port, and set hostname.
 * @param hostname remote host name
 * @param ip new ip to connect to
 * @param port new port to connect to
 * @see connect() Look there for more documentation and notes about possible
 * exceptions.
 */
void
FawkesNetworkClient::connect(const char *hostname, const char *ip, unsigned short int port)
{
  if (__hostname)  free(__hostname);
  if (__ip) free(__ip);
  __hostname = strdup(hostname);
  __ip = ip ? strdup(ip) : NULL;
  __port = port;
  connect();
}

/** Disconnect socket. */
void
FawkesNetworkClient::disconnect()
{
  if ( s == NULL ) return;

  if ( __send_slave_alive ) {
    if ( ! connection_died_recently ) {
      __send_slave->force_send();
      // Give other side some time to read the messages just sent
      usleep(100000);
    }
    __send_slave->cancel();
    __send_slave->join();
    delete __send_slave;
    __send_slave = NULL;
  }
  if ( __recv_slave_alive ) {
    __recv_slave->cancel();
    __recv_slave->join();
    delete __recv_slave;
    __recv_slave = NULL;
  }
  __send_slave_alive = false;
  __recv_slave_alive = false;
  delete s;
  s = NULL;

  if (! connection_died_recently) {
    connection_died();
  }
}


/** Interrupt connect().
 * This is for example handy to interrupt in connection_died() before a
 * connection_established() event has been received.
 */
void
FawkesNetworkClient::interrupt_connect()
{
  __connest_mutex->lock();
  __connest_interrupted = true;
  __connest_mutex->unlock();
  __connest_waitcond->wake_all();
}


/** Enqueue message to send.
 * @param message message to send
 */
void
FawkesNetworkClient::enqueue(FawkesNetworkMessage *message)
{
  if (__send_slave)  __send_slave->enqueue(message);
}


/** Enqueue message to send and wait for answer. It is guaranteed that an
 * answer cannot be missed. However, if the component sends another message
 * (which is not the answer to the query) this will also trigger the wait
 * condition to be woken up. The component ID to wait for is taken from the
 * message.
 * This message also calls unref() on the message. If you want to use it
 * after enqueuing make sure you ref() before calling this method.
 * @param message message to send
 */
void
FawkesNetworkClient::enqueue_and_wait(FawkesNetworkMessage *message)
{
  if (__send_slave && __recv_slave) {
    __recv_mutex->lock();
    __send_slave->enqueue(message);
    if ( waitconds.find(message->cid()) != waitconds.end() ) {
      waitconds[message->cid()]->wait();
    }
    __recv_mutex->unlock();
  }
  message->unref();
}


/** Register handler.
 * Handlers are used to handle incoming packets. There may only be one handler per
 * component!
 * @param handler handler to register
 * @param component_id component ID to register the handler for.
 */
void
FawkesNetworkClient::register_handler(FawkesNetworkClientHandler *handler,
				      unsigned int component_id)
{
  handlers.lock();
  if ( handlers.find(component_id) != handlers.end() ) {
    handlers.unlock();
    throw HandlerAlreadyRegisteredException();
  } else {
    handlers[component_id] = handler;
    waitconds[component_id] = new WaitCondition(__recv_mutex);
  }
  handlers.unlock();
}


/** Deregister handler.
 * @param component_id component ID
 */
void
FawkesNetworkClient::deregister_handler(unsigned int component_id)
{
  handlers.lock();
  if ( handlers.find(component_id) != handlers.end() ) {
    handlers[component_id]->deregistered(_id);
    handlers.erase(component_id);
  }
  if ( waitconds.find(component_id) != waitconds.end() ) {
    WaitCondition *wc = waitconds[component_id];
    waitconds.erase(component_id);
    wc->wake_all();
    delete wc;
  }
  handlers.unlock();
}


void
FawkesNetworkClient::dispatch_message(FawkesNetworkMessage *m)
{
  unsigned int cid = m->cid();
  if (handlers.find(cid) != handlers.end()) {
    handlers[cid]->inbound_received(m, _id);
  }
}


void
FawkesNetworkClient::wake_handlers(unsigned int cid)
{
  if ( waitconds.find(cid) != waitconds.end() ) {
    waitconds[cid]->wake_all();
  }
}

void
FawkesNetworkClient::notify_of_connection_dead()
{
  __connest_mutex->lock();
  __connest = false;
  __connest_mutex->unlock();

  for ( HandlerMap::iterator i = handlers.begin(); i != handlers.end(); ++i ) {
    i->second->connection_died(_id);
  }
  for ( WaitCondMap::iterator j = waitconds.begin(); j != waitconds.end(); ++j) {
    j->second->wake_all();
  }
}

void
FawkesNetworkClient::notify_of_connection_established()
{
  for ( HandlerMap::iterator i = handlers.begin(); i != handlers.end(); ++i ) {
    i->second->connection_established(_id);
  }
  for ( WaitCondMap::iterator j = waitconds.begin(); j != waitconds.end(); ++j) {
    j->second->wake_all();
  }
}


void
FawkesNetworkClient::connection_died()
{
  connection_died_recently = true;
  notify_of_connection_dead();
}


void
FawkesNetworkClient::set_send_slave_alive()
{
  slave_status_mutex->lock();
  __send_slave_alive = true;
  if ( __send_slave_alive && __recv_slave_alive ) {
    __connest_mutex->lock();
    __connest = true;
    __connest_mutex->unlock();
    __connest_waitcond->wake_all();
  }
  slave_status_mutex->unlock();
}


void
FawkesNetworkClient::set_recv_slave_alive()
{
  slave_status_mutex->lock();
  __recv_slave_alive = true;
  if ( __send_slave_alive && __recv_slave_alive ) {
    __connest_mutex->lock();
    __connest = true;
    __connest_mutex->unlock();
    __connest_waitcond->wake_all();
  }
  slave_status_mutex->unlock();
}


/** Wait for messages for component ID.
 * This will wait for messages of the given component ID to arrive. The calling
 * thread is blocked until messages are available.
 * @param component_id component ID to monitor
 */
void
FawkesNetworkClient::wait(unsigned int component_id)
{
  if ( waitconds.find(component_id) != waitconds.end() ) {
    waitconds[component_id]->wait();
  }
}


/** Wake a waiting thread.
 * This will wakeup all threads currently waiting for the specified component ID.
 * This can be helpful to wake a sleeping thread if you received a signal.
 * @param component_id component ID for threads to wake up
 */
void
FawkesNetworkClient::wake(unsigned int component_id)
{
  if ( waitconds.find(component_id) != waitconds.end() ) {
    waitconds[component_id]->wake_all();
  }
}


/** Check if connection is alive.
 * @return true if connection is alive at the moment, false otherwise
 */
bool
FawkesNetworkClient::connected() const throw()
{
  return (! connection_died_recently && (s != NULL));
}


/** Check whether the client has an id.
 * @return true if client has an ID
 */
bool
FawkesNetworkClient::has_id() const
{
  return _has_id;
}


/** Get the client's ID.
 * @return the ID
 */
unsigned int
FawkesNetworkClient::id() const
{
  if ( !_has_id ) {
    throw Exception("Trying to get the ID of a client that has no ID");
  }

  return _id;
}

/** Get the client's hostname
 * @return hostname or NULL
 */
const char *
FawkesNetworkClient::get_hostname() const
{
  return __hostname;
}

/** Get the client's ip
 * @return ip or NULL
 */
const char *
FawkesNetworkClient::get_ip() const
{
  return __ip;
}

} // end namespace fawkes
