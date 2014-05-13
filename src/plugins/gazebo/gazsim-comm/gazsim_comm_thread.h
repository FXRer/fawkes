/***************************************************************************
 *  gazsim_comm_plugin.cpp - Plugin simulates peer-to-peer communication over
 *                    an network with configurable instability and manages
 *                    the frowarding of messages to different ports on
 *                    the same machine.
 *
 *  Created: Thu Sep 12 11:07:43 2013
 *  Copyright  2013  Frederik Zwilling
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

#ifndef __PLUGINS_GAZSIM_COMM_COMM_THREAD_H_
#define __PLUGINS_GAZSIM_COMM_COMM_THREAD_H_

#include <core/threading/thread.h>
#include <aspect/logging.h>
#include <aspect/configurable.h>
#include <aspect/blocked_timing.h>
#include <boost/asio.hpp>
#include <google/protobuf/message.h>
#include <protobuf_comm/peer.h>
#include <protobuf_comm/message_register.h>
#include <list>

namespace protobuf_comm {
  class ProtobufStreamClient;
}

class GazsimCommThread
: public fawkes::Thread,
  public fawkes::BlockedTimingAspect,
  public fawkes::ConfigurableAspect,
  public fawkes::LoggingAspect
{
 public:
  GazsimCommThread();
  ~GazsimCommThread();

  virtual void init();
  virtual void loop();
  virtual void finalize();

  void receive_msg(boost::asio::ip::udp::endpoint &endpoint,
		       uint16_t component_id, uint16_t msg_type,
		       std::shared_ptr<google::protobuf::Message> msg);

 /** Stub to see name in backtrace for easier debugging. @see Thread::run() */
 protected: virtual void run() { Thread::run(); }

 private:
  std::vector<protobuf_comm::ProtobufBroadcastPeer*> peers_;

  //config values
  std::vector<std::string> addresses_;
  std::vector<unsigned int> send_ports_;
  std::vector<unsigned int> recv_ports_;
  //std::string address_;
  //unsigned int send_port_;
  //unsigned int recv_port_;
  std::vector<std::string> proto_dirs_;
  double package_loss_;

  //helper variables
  bool initialized_;
};

#endif
