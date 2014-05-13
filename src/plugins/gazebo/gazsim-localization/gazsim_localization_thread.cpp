/***************************************************************************
 *  gazsim_localization_thread.h - Thread provides 
 *     the simulated position of a robot in Gazeo
 *
 *  Created: Thu Aug 08 17:45:31 2013
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

#include "gazsim_localization_thread.h"

#include <tf/types.h>
#include <stdio.h>
#include <math.h>
#include <utils/math/angle.h>
#include <core/threading/mutex_locker.h>

#include <interfaces/Position3DInterface.h>

#include <gazebo/transport/Node.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/transport.hh>
#include <aspect/logging.h>

using namespace fawkes;
using namespace gazebo;

/** @class LocalizationSimThread "gazsim_localization_thread.h"
 * Thread simulates the Localization in Gazebo
 * @author Frederik Zwilling
 */

/** Constructor. */
LocalizationSimThread::LocalizationSimThread()
  : Thread("LocalizationSimThread", Thread::OPMODE_WAITFORWAKEUP),
    BlockedTimingAspect(BlockedTimingAspect::WAKEUP_HOOK_WORLDSTATE)
{
}

void LocalizationSimThread::init()
{
  logger->log_debug(name(), "Initializing Simulation of the Localization");

  //open interface
  localization_if_ = blackboard->open_for_writing<Position3DInterface>("Pose");

  //subscribing to gazebo publisher
  localization_sub_ = gazebonode->Subscribe(std::string("~/gazsim/gps/"), &LocalizationSimThread::on_localization_msg, this);

  new_data_ = false;
}

void LocalizationSimThread::finalize()
{
  blackboard->close(localization_if_);
}

void LocalizationSimThread::loop()
{
  if(new_data_)
  {
    //write interface
    localization_if_->set_translation(0, x_);
    localization_if_->set_translation(1, y_);
    localization_if_->set_translation(2, z_);
    localization_if_->set_rotation(0, quat_x_);
    localization_if_->set_rotation(1, quat_y_);
    localization_if_->set_rotation(2, quat_z_);
    localization_if_->set_rotation(3, quat_w_);
    localization_if_->write();

    new_data_ = false;
  }
}

void LocalizationSimThread::on_localization_msg(ConstPosePtr &msg)
{
  //logger->log_info(name(), "Got new Localization data.\n");
  
  MutexLocker lock(loop_mutex);

  //read data from message
  x_ = msg->position().x();
  y_ = msg->position().y();
  z_ = msg->position().z();
  quat_x_ = msg->orientation().x();
  quat_y_ = msg->orientation().y();
  quat_z_ = msg->orientation().z();
  quat_w_ = msg->orientation().w();
  
  new_data_ = true;
}
