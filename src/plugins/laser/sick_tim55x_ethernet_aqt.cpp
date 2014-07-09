
/***************************************************************************
 *  sick_tim55x_ethernet_aqt.cpp - Retrieve data from Sick TiM55x via Ethernet
 *
 *  Created: Sun Jun 15 20:45:42 2014
 *  Copyright  2008-2014  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include "sick_tim55x_ethernet_aqt.h"

#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>
#include <utils/misc/string_split.h>
#include <utils/math/angle.h>

#include <boost/lexical_cast.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#if BOOST_VERSION < 104800
#  include <boost/bind.hpp>
#endif
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace fawkes;

#define RECONNECT_INTERVAL   1000
#define RECEIVE_TIMEOUT      500

/** @class SickTiM55xEthernetAcquisitionThread "sick_tim55x_ethernet_aqt.h"
 * Laser acqusition thread for Sick TiM55x laser range finders.
 * This thread fetches the data from the laser.
 * @author Tim Niemueller
 */


/** Constructor.
 * @param cfg_name short name of configuration group
 * @param cfg_prefix configuration path prefix
 */
SickTiM55xEthernetAcquisitionThread::SickTiM55xEthernetAcquisitionThread(std::string &cfg_name,
							       std::string &cfg_prefix)
  : SickTiM55xCommonAcquisitionThread(cfg_name, cfg_prefix),
    socket_(io_service_), deadline_(io_service_)
{
  set_name("SickTiM55x(%s)", cfg_name.c_str());
}

void
SickTiM55xEthernetAcquisitionThread::init()
{
  pre_init(config, logger);
  read_common_config();

  cfg_host_ = config->get_string((cfg_prefix_ + "host").c_str());
  cfg_port_ = config->get_string((cfg_prefix_ + "port").c_str());

  socket_mutex_ = new Mutex();

  deadline_.expires_at(boost::posix_time::pos_infin);
  check_deadline();

  init_device();
}


void
SickTiM55xEthernetAcquisitionThread::finalize()
{
  free(_distances);
  _distances = NULL;

  delete socket_mutex_;
}


void
SickTiM55xEthernetAcquisitionThread::loop()
{
  if (socket_.is_open()) {
    try {
      deadline_.expires_from_now(boost::posix_time::milliseconds(RECEIVE_TIMEOUT));

      boost::system::error_code ec = boost::asio::error::would_block;
      size_t bytes_read = 0;
      boost::asio::async_read_until(socket_, input_buffer_, '\03',
				    (boost::lambda::var(ec) = boost::lambda::_1,
				     boost::lambda::var(bytes_read) = boost::lambda::_2));

      do io_service_.run_one(); while (ec == boost::asio::error::would_block);

      reset_distances();
      reset_echoes();

      if (ec) {
	if (ec.value() == boost::system::errc::operation_canceled) {
	  logger->log_error(name(), "Data timeout, will try to reconnect");
	} else {
	  logger->log_warn(name(), "Data read error: %s\n", ec.message().c_str());
	}
	_data_mutex->lock();
	_timestamp->stamp();
	_new_data = true;
	_data_mutex->unlock();
	close_device();

      } else {
	unsigned char recv_buf[bytes_read];
	std::istream in_stream(&input_buffer_);
	in_stream.read((char *)recv_buf, bytes_read);

	if (bytes_read > 0) {
	  try {
	    parse_datagram(recv_buf, bytes_read);
	  } catch (Exception &e) {
	    logger->log_warn(name(), "Failed to parse datagram, resyncing, exception follows");
	    logger->log_warn(name(), e);
	    resync();
	  }
	}
      }
    } catch (boost::system::system_error &e) {
      if (e.code() == boost::asio::error::eof) {
	close_device();
	logger->log_warn(name(),
			 "Sick TiM55x/Ethernet connection lost, trying to reconnect");
      } else {
	logger->log_warn(name(), "Sick TiM55x/Ethernet failed read: %s", e.what());
      }
    }
  } else {
    try {
      init_device();
      logger->log_warn(name(), "Reconnected to device");
    } catch (Exception &e) {
      // ignore, keep trying
      usleep(RECONNECT_INTERVAL * 1000);
    }
  }

  yield();
}


void
SickTiM55xEthernetAcquisitionThread::open_device()
{
  try {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query
      query(cfg_host_, cfg_port_);
    boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

    // this is just the overly complicated way to get a timeout on
    // a synchronous connect, cf.
    // http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/example/cpp03/timeouts/blocking_tcp_client.cpp

    deadline_.expires_from_now(boost::posix_time::seconds(5));

    boost::system::error_code ec;

    for (; iter != boost::asio::ip::tcp::resolver::iterator(); ++iter) {
      socket_.close();
      ec = boost::asio::error::would_block;
      socket_.async_connect(iter->endpoint(), boost::lambda::var(ec) = boost::lambda::_1);

      // Block until the asynchronous operation has completed.
      do io_service_.run_one(); while (ec == boost::asio::error::would_block);

      // Determine whether a connection was successfully established.
      if (ec || ! socket_.is_open()) {
	if (ec.value() == boost::system::errc::operation_canceled) {
	  throw Exception("Sick TiM55X Ethernet: connection timed out");
	} else {
	  throw Exception("Connection failed: %s", ec.message().c_str());
	}
      }				       
    }
  } catch (boost::system::system_error &e) {
    throw Exception("Connection failed: %s", e.what());
  }
}


void
SickTiM55xEthernetAcquisitionThread::close_device()
{
  boost::system::error_code err;
  if (socket_.is_open()) {
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, err);
    socket_.close();
  }
}


void
SickTiM55xEthernetAcquisitionThread::flush_device()
{
  if (socket_.is_open()) {
    try {
      boost::system::error_code ec;
      size_t bytes_read = 0;
      do {
	ec = boost::asio::error::would_block;
	deadline_.expires_from_now(boost::posix_time::milliseconds(RECEIVE_TIMEOUT));
	bytes_read = 0;

	boost::asio::async_read_until(socket_, input_buffer_, '\03',
				      (boost::lambda::var(ec) = boost::lambda::_1,
				       boost::lambda::var(bytes_read) = boost::lambda::_2));

	do io_service_.run_one(); while (ec == boost::asio::error::would_block);

      } while (ec || bytes_read > 0);
    } catch (boost::system::system_error &e) {
      // ignore, just assume done, if there really is an error we'll
      // catch it later on
    }
  }
}

void
SickTiM55xEthernetAcquisitionThread::send_with_reply(const char *request,
						     std::string *reply)
{
  MutexLocker lock(socket_mutex_);

  int request_length = strlen(request);

  try {
    boost::asio::write(socket_, boost::asio::buffer(request, request_length));

    deadline_.expires_from_now(boost::posix_time::milliseconds(RECEIVE_TIMEOUT));

    boost::system::error_code ec = boost::asio::error::would_block;
    size_t bytes_read = 0;
    boost::asio::async_read_until(socket_, input_buffer_, '\03',
				  (boost::lambda::var(ec) = boost::lambda::_1,
				   boost::lambda::var(bytes_read) = boost::lambda::_2));

    do io_service_.run_one(); while (ec == boost::asio::error::would_block);

    if (ec) {
      if (ec.value() == boost::system::errc::operation_canceled) {
	throw Exception("Timeout waiting for message reply");
      } else {
	throw Exception("Failed to read reply: %s", ec.message().c_str());
      }
    }

    if (reply) {
      char recv_buf[bytes_read];
      std::istream in_stream(&input_buffer_);
      in_stream.read(recv_buf, bytes_read);
      *reply = std::string(recv_buf, bytes_read);
    } else {
      input_buffer_.consume(bytes_read);
    }
  } catch (boost::system::system_error &e) {
    throw Exception("Sick TiM55x/Ethernet failed I/O: %s", e.what());
  }
}


/** Check whether the deadline has passed.
 * We compare the deadline against
 * the current time since a new asynchronous operation may have moved the
 * deadline before this actor had a chance to run.
 */
void
SickTiM55xEthernetAcquisitionThread::check_deadline()
{
  if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now()) {
    socket_.close();
    deadline_.expires_at(boost::posix_time::pos_infin);
  }

  deadline_.async_wait(boost::lambda::bind(&SickTiM55xEthernetAcquisitionThread::check_deadline, this));
}
