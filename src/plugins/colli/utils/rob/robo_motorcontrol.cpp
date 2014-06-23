
/***************************************************************************
 *  robo_motorcontrol.cpp - Motor control wrapper
 *
 *  Created: Fri Oct 18 15:16:23 2013
 *  Copyright  2002  Stefan Jacobs
 *             2013  Bahram Maleki-Fard
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

#include "robo_motorcontrol.h"

#include <interfaces/MotorInterface.h>
#include <utils/math/angle.h>

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class MotorControl <plugins/colli/utils/rob/robo_motorcontrol.cpp>
 * This class is an interface to the obligatory MotorControl in the BlackBoard.
 */

MotorControl::MotorControl( fawkes::MotorInterface* motor )
{
  m_pMopo = motor;
  SetEmergencyStop();
}



MotorControl::~MotorControl()
{
}


float MotorControl::GetCurrentX()
{
  return m_pMopo->odometry_position_x();
}


float MotorControl::GetCurrentY()
{
  return m_pMopo->odometry_position_y();
}


float MotorControl::GetCurrentOri()
{
  return normalize_mirror_rad( m_pMopo->odometry_orientation() );
}


float MotorControl::GetMotorDesiredTranslation()
{
  float vx = m_pMopo->des_vx();
  float vy = m_pMopo->des_vy();
  float speed = sqrt(vx*vx + vy*vy);

  if ( vx > 0 )
    return speed;
  else
    return -speed;
}


float MotorControl::GetMotorDesiredRotation()
{
  return m_pMopo->des_omega();
}


float MotorControl::GetMotorCurrentTranslation()
{
  float vx = m_pMopo->vx();
  float vy = m_pMopo->vy();
  float speed = sqrt(vx*vx + vy*vy);

  if ( vx > 0 )
    return speed;
  else
    return -speed;
}


float MotorControl::GetMotorCurrentRotation()
{
  return m_pMopo->omega();
}



float MotorControl::GetUserDesiredTranslation()
{
  return m_MotorControlDesiredTranslation;
}


float MotorControl::GetUserDesiredRotation()
{
  return m_MotorControlDesiredRotation;
}


bool MotorControl::GetMovingAllowed()
{
  return m_pMopo->motor_state() == MotorInterface::MOTOR_ENABLED;
}


void MotorControl::SetDesiredTranslation( float speed )
{
  m_MotorControlDesiredTranslation = speed;
}


void MotorControl::SetDesiredRotation( float ori )
{
  m_MotorControlDesiredRotation = ori;
}


bool MotorControl::SendCommand()
{
  if ( m_MovingAllowed ) {
    m_pMopo->msgq_enqueue(new MotorInterface::TransRotMessage(m_MotorControlDesiredTranslation, 0,
                                                              m_MotorControlDesiredRotation));
    return true;
  } else {
    return false;
  }
}


void MotorControl::SetEmergencyStop()
{
  m_MovingAllowed = false;
  m_pMopo->msgq_enqueue(new MotorInterface::SetMotorStateMessage(MotorInterface::MOTOR_DISABLED));
}


void MotorControl::SetRecoverEmergencyStop()
{
  m_MovingAllowed = true;
  m_pMopo->msgq_enqueue(new MotorInterface::SetMotorStateMessage(MotorInterface::MOTOR_ENABLED));
}

} // end of namespace fawkes
