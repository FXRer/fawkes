/***************************************************************************
 *  sim_interface.h - Simulates an Interface
 *
 *  Created: Wed Jun 19 09:27:06 2013
 *  Copyright  2013  Frederik Zwilling
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

#ifndef __SIM_INTERFACE_H_
#define __SIM_INTERFACE_H_

#include <logging/logger.h>
#include <blackboard/blackboard.h>
#include <config/config.h>

//from Gazebo
#include <gazebo/transport/TransportTypes.hh>
#include <gazebo/msgs/MessageTypes.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/transport/Node.hh>


class SimInterface
{
 public:
  /** Constructor
   * @param controlPublisher Publisher for sending control msgs
   * @param logger Logger
   * @param blackboard Blackboard
   * @param gazebonode Gazebo node for communication
   * @param name name of the interface(ID)
   * @param config Fawkes Config
   */
  SimInterface(gazebo::transport::PublisherPtr controlPublisher, fawkes::Logger *logger, fawkes::BlackBoard *blackboard,   gazebo::transport::NodePtr gazebonode, const char* name, fawkes::Configuration *config)
  {
    this->control_pub_ = controlPublisher;
    this->logger_ = logger;
    this->blackboard_ = blackboard;
    this->gazebonode_ = gazebonode;
    this->name_ = name;
    this->config_ = config;
  };
  ///Destructor
  ~SimInterface() {};

  /// initialization of the interface
  virtual void init(){};
  /// update on every loop
  virtual void loop(){};
  /// finalize/close interface
  virtual void finalize(){};
 
 protected:
  ///Name for the logger
  const char* name_;
  ///Logger
  fawkes::Logger *logger_;
  ///Blackboard
  fawkes::BlackBoard *blackboard_;
  ///Gazebo node for communication
  gazebo::transport::NodePtr gazebonode_;
  ///Access to Fawkes Config
  fawkes::Configuration *config_;

  ///Publisher to send control-messages to gazebo
  gazebo::transport::PublisherPtr control_pub_;
};

#endif
