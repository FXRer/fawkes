
/***************************************************************************
 *  quadratic_motor_instruct.cpp - Motor instructor with quadratic approximation
 *
 *  Created: Fri Oct 18 15:16:23 2013
 *  Copyright  2002  Stefan Jacobs
 *             2013  Bahram Maleki-Fard
 *             2014  Tobias Neumann
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

#include "linear_motor_instruct.h"

#include <interfaces/MotorInterface.h>
#include <logging/logger.h>
#include <config/config.h>
#include <utils/math/common.h>

#include <string>

namespace fawkes
{
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

using namespace std;

/** @class CLinearMotorInstruct <plugins/colli/drive_realization/quadratic_motor_instruct.h>
 * This module is a class for validity checks of drive
 * commands and sets those things with respect to the physical
 * borders of the robot.
 * For this purpose the two functions CalculateRotation and
 * CalculateTranslation are implemented quadratically ;-)
 */

/** Constructor.
 * @param motor The MotorInterface with all the motor information
 * @param frequency The frequency of the colli (should become deprecated!)
 * @param logger The fawkes logger
 * @param config The fawkes configuration
 */
CLinearMotorInstruct::CLinearMotorInstruct( fawkes::MotorInterface* motor,
                                                  float frequency,
                                                  fawkes::Logger* logger,
                                                  fawkes::Configuration* config )
 : CBaseMotorInstruct( motor, frequency, logger ),
   config_( config )
{
  logger_->log_debug("CLinearMotorInstruct", "(Constructor): Entering");

  string cfg_prefix = "/plugins/colli/motor_instruct/";

  basic_trans_acc = config_->get_float((cfg_prefix + "trans_acc").c_str());
  basic_trans_dec = config_->get_float((cfg_prefix + "trans_dec").c_str());
  basic_rot_acc   = config_->get_float((cfg_prefix + "rot_acc").c_str());
  basic_rot_dec   = config_->get_float((cfg_prefix + "rot_dec").c_str());

  logger_->log_debug("CLinearMotorInstruct", "(Constructor): Exiting");
}


CLinearMotorInstruct::~CLinearMotorInstruct()
{
  logger_->log_debug("CLinearMotorInstruct", "(Destructor): Entering");
  logger_->log_debug("CLinearMotorInstruct", "(Destructor): Exiting");
}




/** Implementation of Calculate Translation Function.
 * These are dangerous! Take care while modifying. Only a minus sign too few
 *   or too much may result in non predictable motor behaviour!!!!
 * THIS FUNCTION IS THE LAST BORDER TO THE MOTOR, TAKE CARE AND PAY ATTENTION!!!
 */
float CLinearMotorInstruct::CalculateTranslation( float currentTranslation,
                 float desiredTranslation,
                 float time_factor )
{
  float execTranslation = 0.0;

  if (desiredTranslation < currentTranslation) {

    if (currentTranslation > 0.0) {
      // decrease forward speed
      execTranslation = currentTranslation - basic_trans_dec;
      execTranslation = max( execTranslation, desiredTranslation );

    } else if (currentTranslation < 0.0) {
      // increase backward speed
      execTranslation = currentTranslation - basic_trans_acc;
      execTranslation = max( execTranslation, desiredTranslation );

    }  else {
      // currentTranslation == 0;
      execTranslation = max( -basic_trans_acc, desiredTranslation );
    }

  } else if (desiredTranslation > currentTranslation) {

    if (currentTranslation > 0.0) {
      // increase forward speed
      execTranslation = currentTranslation + basic_trans_acc;
      execTranslation = min( execTranslation, desiredTranslation );

    } else if (currentTranslation < 0.0) {
      // decrease backward speed
      execTranslation = currentTranslation + basic_trans_dec;
      execTranslation = min( execTranslation, desiredTranslation );

    } else {
      // currentTranslation == 0
      execTranslation = min( basic_trans_acc, desiredTranslation );
    }

  } else {
    // nothing to change!!!
    execTranslation = desiredTranslation;
  }

  return execTranslation*time_factor;
}




/** Implementation of Calculate Rotation Function.
 * These are dangerous! Take care while modifying. Only a minus sign too few
 *   or too much may result in non predictable motor behaviour!!!!
 * THIS FUNCTION IS THE LAST BORDER TO THE MOTOR, TAKE CARE AND PAY ATTENTION!!!
 */
float CLinearMotorInstruct::CalculateRotation( float currentRotation,
              float desiredRotation,
              float time_factor  )
{
  float execRotation = 0.0;

  if (desiredRotation < currentRotation) {

    if (currentRotation > 0.0) {
      // decrease right rot
      execRotation = currentRotation - basic_rot_dec;
      execRotation = max( execRotation, desiredRotation );

    } else if (currentRotation < 0.0) {
      // increase left rot
      execRotation = currentRotation - basic_rot_acc;
      execRotation = max( execRotation, desiredRotation );

    } else {
      // currentRotation == 0;
      execRotation = max( -basic_rot_acc, desiredRotation );
    }

  } else if (desiredRotation > currentRotation) {
    if (currentRotation > 0.0) {
      // increase right rot
      execRotation = currentRotation + basic_rot_acc;
      execRotation = min( execRotation, desiredRotation );

    } else if (currentRotation < 0.0) {
      // decrease left rot
      execRotation = currentRotation + basic_rot_dec;
      execRotation = min( execRotation, desiredRotation );

    } else {
      // currentRotation == 0
      execRotation = min( basic_rot_acc, desiredRotation );
    }

  } else {
    // nothing to change!!!
    execRotation = desiredRotation;
  }

  return execRotation*time_factor;
}

} // namespace fawkes
