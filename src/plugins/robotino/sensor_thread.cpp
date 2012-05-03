
/***************************************************************************
 *  sensor_thread.cpp - Robotino sensor thread
 *
 *  Created: Sun Nov 13 15:35:24 2011
 *  Copyright  2011-2012  Tim Niemueller [www.niemueller.de]
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

#include "sensor_thread.h"
#include <rec/robotino/com/Com.h>
#include <rec/sharedmemory/sharedmemory.h>
#include <rec/iocontrol/remotestate/SensorState.h>
#include <rec/iocontrol/robotstate/State.h>
#include <baseapp/run.h>

#include <interfaces/BatteryInterface.h>
#include <interfaces/RobotinoSensorInterface.h>

using namespace fawkes;

/** @class RobotinoSensorThread "sensor_thread.h"
 * Robotino sensor hook integration thread.
 * This thread integrates into the Fawkes main loop at the SENSOR hook and
 * writes new sensor data.
 * @author Tim Niemueller
 */

/** Constructor. */
RobotinoSensorThread::RobotinoSensorThread()
  : Thread("RobotinoSensorThread", Thread::OPMODE_WAITFORWAKEUP),
    BlockedTimingAspect(BlockedTimingAspect::WAKEUP_HOOK_SENSOR_ACQUIRE)
{
}


void
RobotinoSensorThread::init()
{
  cfg_hostname_ = config->get_string("/hardware/robotino/hostname");
  cfg_quit_on_disconnect_ = config->get_bool("/hardware/robotino/quit_on_disconnect");

  com_ = new rec::robotino::com::Com();
  com_->setAddress(cfg_hostname_.c_str());
  com_->connect(/* blocking */ false);

  last_seqnum_ = 0;

  batt_if_ = blackboard->open_for_writing<BatteryInterface>("Robotino");
  sens_if_ = blackboard->open_for_writing<RobotinoSensorInterface>("Robotino");

  statemem_ =  new rec::sharedmemory::SharedMemory<rec::iocontrol::robotstate::State>
    (rec::iocontrol::robotstate::State::sharedMemoryKey);
  state_ = statemem_->getData();

  // taken from Robotino API2 DistanceSensorImpl.hpp
  voltage_to_dist_dps_.push_back(std::make_pair(0.3 , 0.41));
  voltage_to_dist_dps_.push_back(std::make_pair(0.39, 0.35));
  voltage_to_dist_dps_.push_back(std::make_pair(0.41, 0.30));
  voltage_to_dist_dps_.push_back(std::make_pair(0.5 , 0.25));
  voltage_to_dist_dps_.push_back(std::make_pair(0.75, 0.18));
  voltage_to_dist_dps_.push_back(std::make_pair(0.8 , 0.16));
  voltage_to_dist_dps_.push_back(std::make_pair(0.95, 0.14));
  voltage_to_dist_dps_.push_back(std::make_pair(1.05, 0.12));
  voltage_to_dist_dps_.push_back(std::make_pair(1.3 , 0.10));
  voltage_to_dist_dps_.push_back(std::make_pair(1.4 , 0.09));
  voltage_to_dist_dps_.push_back(std::make_pair(1.55, 0.08));
  voltage_to_dist_dps_.push_back(std::make_pair(1.8 , 0.07));
  voltage_to_dist_dps_.push_back(std::make_pair(2.35, 0.05));
  voltage_to_dist_dps_.push_back(std::make_pair(2.55, 0.04));
}


void
RobotinoSensorThread::finalize()
{
  blackboard->close(sens_if_);
  blackboard->close(batt_if_);
  delete com_;
}

void
RobotinoSensorThread::loop()
{
  if (com_->isConnected()) {
    rec::iocontrol::remotestate::SensorState sensor_state = com_->sensorState();
    if (sensor_state.sequenceNumber != last_seqnum_) {
      last_seqnum_ = sensor_state.sequenceNumber;

      // update sensor values in interface
      sens_if_->set_mot_velocity(sensor_state.actualVelocity);
      sens_if_->set_mot_position(sensor_state.actualPosition);
      sens_if_->set_mot_current(sensor_state.motorCurrent);
      sens_if_->set_bumper(sensor_state.bumper);
      sens_if_->set_digital_in(sensor_state.dIn);
      sens_if_->set_analog_in(sensor_state.aIn);
      if (state_->gyro.port == rec::serialport::UNDEFINED) {
        if (sens_if_->is_gyro_available()) {
          sens_if_->set_gyro_available(false);
          sens_if_->set_gyro_angle(0.);
          sens_if_->set_gyro_rate(0.);
        }
      } else {
        sens_if_->set_gyro_available(true);
        sens_if_->set_gyro_angle(state_->gyro.angle);
        sens_if_->set_gyro_rate(state_->gyro.rate);
      }

      update_distances(sensor_state.distanceSensor);

      sens_if_->write();

      batt_if_->set_voltage(roundf(sensor_state.voltage * 1000.));
      batt_if_->set_current(roundf(sensor_state.current));

      // 21.0V is empty, 26.0V is empty, from OpenRobotino lcdd
      float soc = (sensor_state.voltage - 21.0f) / 5.f;
      soc = std::min(1.f, std::max(0.f, soc));

      batt_if_->set_absolute_soc(soc);
      batt_if_->write();

    }
  } else if (com_->connectionState() == rec::robotino::com::Com::NotConnected) {
    // retry connection
    if (cfg_quit_on_disconnect_) {
      logger->log_warn(name(), "Connection lost, quitting (as per config)");
      fawkes::runtime::quit();
    } else {
      com_->connect(/* blocking */ false);
    }
  }
}


void
RobotinoSensorThread::update_distances(float *voltages)
{
  float dist_m[NUM_IR_SENSORS];
  const size_t num_dps = voltage_to_dist_dps_.size();

  for (int i = 0; i < NUM_IR_SENSORS; ++i) {
    dist_m[i] = 0.;
    // find the two enclosing data points

    for (size_t j = 0; j < num_dps - 1; ++j) {
      // This determines two points, l(eft) and r(ight) that are
      // defined by voltage (x coord) and distance (y coord). We
      // assume a linear progression between two adjacent points,
      // i.e. between l and r. We then do the following:
      // 1. Find two adjacent voltage values lv and rv where
      //    the voltage lies inbetween
      // 2. Interpolate by calculating the line parameters
      //    m = dd/dv, x = voltage - lv and b = ld.
      // cf. http://www.acroname.com/robotics/info/articles/irlinear/irlinear.html

      const double lv = voltage_to_dist_dps_[j  ].first;
      const double rv = voltage_to_dist_dps_[j+1].first;

      if ((voltages[i] >= lv) && (voltages[i] < rv)) {
        const double ld = voltage_to_dist_dps_[j  ].second;
        const double rd = voltage_to_dist_dps_[j+1].second;

        double dv = rv - lv;
        double dd = rd - ld;

        // Linear interpolation between 
        dist_m[i] = (dd / dv) * (voltages[i] - lv) + ld;
        break;
      }
    }
  }

  sens_if_->set_distance(dist_m);
}
