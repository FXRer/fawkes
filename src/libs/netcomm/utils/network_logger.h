
/***************************************************************************
 *  network_logger.h - Fawkes network logger
 *
 *  Created: Sat Dec 15 00:45:54 2007 (after I5 xmas party)
 *  Copyright  2006-2007  Tim Niemueller [www.niemueller.de]
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

#ifndef __NETCOMM_UTILS_NETWORK_LOGGER_H_
#define __NETCOMM_UTILS_NETWORK_LOGGER_H_

#include <core/utils/lock_list.h>
#include <core/utils/lock_queue.h>
#include <utils/logging/logger.h>
#include <netcomm/fawkes/handler.h>
#include <netcomm/fawkes/message_content.h>

#include <stdint.h>

namespace fawkes {

class Mutex;
class FawkesNetworkHub;

class NetworkLogger
: public Logger,
  public FawkesNetworkHandler
{
 public:
  NetworkLogger(FawkesNetworkHub *hub, LogLevel log_level = LL_DEBUG);
  virtual ~NetworkLogger();

  virtual void log_debug(const char *component, const char *format, ...);
  virtual void log_info(const char *component, const char *format, ...);
  virtual void log_warn(const char *component, const char *format, ...);
  virtual void log_error(const char *component, const char *format, ...);

  virtual void log_debug(const char *component, Exception &e);
  virtual void log_info(const char *component, Exception &e);
  virtual void log_warn(const char *component, Exception &e);
  virtual void log_error(const char *component, Exception &e);

  virtual void vlog_debug(const char *component, const char *format, va_list va);
  virtual void vlog_info(const char *component, const char *format, va_list va);
  virtual void vlog_warn(const char *component, const char *format, va_list va);
  virtual void vlog_error(const char *component, const char *format, va_list va);

  virtual void tlog_debug(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_info(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_warn(struct timeval *t, const char *component, const char *format, ...);
  virtual void tlog_error(struct timeval *t, const char *component, const char *format, ...);

  virtual void tlog_debug(struct timeval *t, const char *component, Exception &e);
  virtual void tlog_info(struct timeval *t, const char *component, Exception &e);
  virtual void tlog_warn(struct timeval *t, const char *component, Exception &e);
  virtual void tlog_error(struct timeval *t, const char *component, Exception &e);

  virtual void vtlog_debug(struct timeval *t, const char *component,
			   const char *format, va_list va);
  virtual void vtlog_info(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_warn(struct timeval *t, const char *component,
			  const char *format, va_list va);
  virtual void vtlog_error(struct timeval *t, const char *component,
			   const char *format, va_list va);

  virtual void handle_network_message(FawkesNetworkMessage *msg);
  virtual void client_connected(unsigned int clid);
  virtual void client_disconnected(unsigned int clid);

  /** NetworkLogger message types. */
  typedef enum {
    MSGTYPE_SUBSCRIBE   = 1,	/**< Subscribe for logging messages. */
    MSGTYPE_UNSUBSCRIBE = 2,	/**< Unsubscribe from receiving logging messages. */
    MSGTYPE_LOGMESSAGE  = 3	/**< Log message. */
  } network_logger_msgtype_t;

  /** Network logging message header. */
  typedef struct {
    uint32_t        log_level :  2;	/**< LogLevel, @see Logger::LogLevel */
    uint32_t        reserved  : 30;	/**< reserved for future use */
    struct timeval  time;		/**< time, sec and usec are encoded in network byte order */
  } network_logger_header_t;

 private:
  void send_message(Logger::LogLevel level, struct timeval *t,
		    const char *component, const char *format, va_list va);
  void send_message(Logger::LogLevel level, struct timeval *t,
		    const char *component, const char *message);

  FawkesNetworkHub *hub;

  LockQueue< FawkesNetworkMessage * > inbound_queue;

  LockList<unsigned int>           __subscribers;
  LockList<unsigned int>::iterator __ssit;
};


class NetworkLoggerMessageContent : public FawkesNetworkMessageContent
{
 public:
  NetworkLoggerMessageContent(Logger::LogLevel log_level, struct timeval *t,
			      const char *component, const char *message);
  NetworkLoggerMessageContent(Logger::LogLevel log_level, struct timeval *t,
			      const char *component, const char *format, va_list va);
  NetworkLoggerMessageContent(const NetworkLoggerMessageContent *content);
  NetworkLoggerMessageContent(unsigned int component_id, unsigned int msg_id,
			      void *payload, size_t payload_size);
  virtual ~NetworkLoggerMessageContent();

  struct timeval    get_time() const;
  Logger::LogLevel  get_loglevel() const;
  const char *      get_component() const;
  const char *      get_message() const;

  virtual void   serialize();

 private:
  NetworkLogger::network_logger_header_t *header;
  const char *__component;
  const char *__message;
};

} // end namespace fawkes

#endif
