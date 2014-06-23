
/***************************************************************************
 *  fast_biward_drive_mode.cpp - Implementation of drive-mode "fast forward + backward"
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

#include "fast_biward_drive_mode.h"

namespace fawkes
{
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** Initialize your local values here.
 */
CFastBiwardDriveModule::CFastBiwardDriveModule( CFastForwardDriveModule*  forward_module,
                                                CFastBackwardDriveModule* backward_module,
                                                Logger* logger,
                                                Configuration* config )
 : CAbstractDriveMode(logger, config)
{
  logger_->log_info("CFastBiwardDriveModule", "(Constructor): Entering...");
  m_DriveModeName = NavigatorInterface::FastAllowBackward;
  m_pFastForwardDriveModule  = forward_module;
  m_pFastBackwardDriveModule = backward_module;

  m_CountForward = 1;

  m_MaxTranslation = config_->get_float( "/plugins/colli/FastDriveModule/MAX_TRANS" );
  m_MaxRotation    = config_->get_float( "/plugins/colli/FastDriveModule/MAX_ROT" );

  logger_->log_info("CFastBiwardDriveModule", "(Constructor): Exiting...");
}


/** Destruct your local values here.
 */
CFastBiwardDriveModule::~CFastBiwardDriveModule()
{
  logger_->log_info("CFastBiwardDriveModule", "(Destructor): Entering...");
  m_DriveModeName = NavigatorInterface::MovingNotAllowed;
  logger_->log_info("CFastBiwardDriveModule", "(Destructor): Exiting...");
}



/* ************************************************************************** */
/* ***********************        U P D A T E       ************************* */
/* ************************************************************************** */

/** Calculate here your desired settings. What you desire is checked afterwards to the current
 *    settings of the physical boundaries, but take care also.
 *
 *  How you do this is up to you, but be careful, our hardware is expensive!!!!
 *
 *  Available are:
 *
 *     m_TargetX, m_TargetY, m_TargetOri  --> current Target to drive to
 *     m_RoboX, m_RoboY, m_RoboOri        --> current Robot coordinates
 *     m_RoboTrans, m_RoboRot             --> current Motor values
 *
 *     m_LocalTargetX, m_LocalTargetY     --> our local target found by the search component we want to reach
 *     m_LocalTrajecX, m_LocalTrajecY     --> The point we would collide with, if we would drive WITHOUT Rotation
 *
 *     m_OrientAtTarget                   --> Do we have to orient ourself at the target?
 *     m_StopAtTarget                     --> Do we have to stop really ON the target?
 *
 *  Afterwards filled should be:
 *
 *     m_ProposedTranslation              --> Desired Translation speed
 *     m_ProposedRotation                 --> Desired Rotation speed
 *
 *  Those values are questioned after an Update() was called.
 */
void
CFastBiwardDriveModule::Update()
{
  m_ProposedTranslation = 0.0;
  m_ProposedRotation    = 0.0;

  // Our drive mode (choose between forward and backward)
  CAbstractDriveMode * driveMode = 0;

  // Search the correct drive mode
  float angle_to_target = atan2( m_LocalTargetY, m_LocalTargetX );

  if ( m_CountForward == 1 && fabs( angle_to_target ) > M_PI_2+0.1 )
    m_CountForward = -1;

  else if ( m_CountForward == 1 )
    m_CountForward = 1;

  else if ( m_CountForward == -1 && fabs( angle_to_target ) < M_PI_2-0.1 )
    m_CountForward = 1;

  else if ( m_CountForward == -1 )
    m_CountForward = -1;

  else {
    logger_->log_debug("CFastBiwardDriveModule", "Undefined state");
    m_CountForward = 0;
  }

  if ( m_CountForward == 1 )
    driveMode = m_pFastForwardDriveModule;
  else
    driveMode = m_pFastBackwardDriveModule;



  // set the current info to the drive mode
  driveMode->SetCurrentRoboPos( m_RoboX, m_RoboY, m_RoboOri );
  driveMode->SetCurrentRoboSpeed( m_RoboTrans, m_RoboRot );
  driveMode->SetCurrentTarget( m_TargetX, m_TargetY, m_TargetOri );
  driveMode->SetLocalTarget( m_LocalTargetX, m_LocalTargetY );
  driveMode->SetLocalTrajec( m_LocalTrajecX, m_LocalTrajecY );
  driveMode->SetCurrentColliMode( m_OrientAtTarget, m_StopAtTarget );

  // update the drive mode
  driveMode->Update();

  // get the values from the drive mode
  m_ProposedTranslation = driveMode->GetProposedTranslation();
  m_ProposedRotation    = driveMode->GetProposedRotation();

}

} // namespace fawkes
