
/***************************************************************************
 *  abstract_drive_mode.h - Abstract base class for a drive-mode
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

#ifndef __PLUGINS_COLLI_ABSTRACT_DRIVE_MODE_H_
#define __PLUGINS_COLLI_ABSTRACT_DRIVE_MODE_H_

#include <interfaces/NavigatorInterface.h>
#include <logging/logger.h>
#include <config/config.h>

#include <cmath>

namespace fawkes
{
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class CAbstractDriveMode <plugins/colli/drive_modes/abstract_drive_mode.h>
 * This is the base class which calculates drive modes. Drive modes are the
 * proposed settings for the drive-realization out of the found search things.
 */
class CAbstractDriveMode
{
 public:

  CAbstractDriveMode(fawkes::Logger* logger, fawkes::Configuration* config);
  virtual ~CAbstractDriveMode();

  ///\brief Sets the current target.
  void SetCurrentTarget( float targetX, float targetY, float targetOri );

  ///\brief Sets the current robo position.
  void SetCurrentRoboPos( float roboX, float roboY, float roboOri );

  ///\brief Sets the current robo speed.
  void SetCurrentRoboSpeed( float roboTrans, float roboRot );

  ///\brief Set the colli mode values for each drive mode.
  void SetCurrentColliMode( bool orient, bool stop );

  ///\brief Set the local targetpoint found by the search.
  void SetLocalTarget( float localTargetX, float localTargetY );

  ///\brief  Set the local trajectory point found by the search.
  void SetLocalTrajec( float localTrajecX, float localTrajecY );

  ///\brief Returns the drive modes name.
  NavigatorInterface::DriveMode GetDriveModeName();


  ///\brief Calculate the proposed settings which are asked for afterwards.
  virtual void Update() = 0;

  ///\brief Returns the proposed translation
  float GetProposedTranslation();

  ///\brief Returns the proposed rotatio
  float GetProposedRotation();


 protected:

  ///\brief Perform linear interpolation
  float LinInterpol( float x, float left, float right, float bot, float top );

  ///\brief Get velocity that guarantees a stop for a given distance
  float GuaranteeTransStop( float distance, float current_trans, float desired_trans );

  float m_TargetX;    /**< current target x */
  float m_TargetY;    /**< current target y */
  float m_TargetOri;  /**< current target ori */

  float m_RoboX;      /**< current robo pos x */
  float m_RoboY;      /**< current robo pos y */
  float m_RoboOri;    /**< current robo ori */

  float m_RoboTrans;  /**< current robo translation velocity */
  float m_RoboRot;    /**< current robo rotation velocity */

  float m_LocalTargetX;  /**< local target x */
  float m_LocalTargetY;  /**< local target y */

  float m_LocalTrajecX;  /**< local trajectory x */
  float m_LocalTrajecY;  /**< local trajectory y*/

  bool m_OrientAtTarget; /**< flag if orienting necessary */
  bool m_StopAtTarget;   /**< flag if stopping on or after target */

  float m_ProposedTranslation; /**< proposed translation setting for next timestep */
  float m_ProposedRotation;    /**< proposed rotation setting for next timestep */

  fawkes::NavigatorInterface::DriveMode m_DriveModeName;  /**< the drive mode name */

  fawkes::Logger* logger_;          /**< The fawkes logger */
  fawkes::Configuration* config_;   /**< The fawkes configuration */

private:

  float m_cMaxTransDec;
  float m_cMaxRotDec;
};



/* ************************************************************************** */
/* ***********************  IMPLEMENTATION DETAILS  ************************* */
/* ************************************************************************** */

/** Constructor.
 * @param logger The fawkes logger
 * @param config The fawkes configuration
 */
inline
CAbstractDriveMode::CAbstractDriveMode(fawkes::Logger* logger, fawkes::Configuration* config)
 : logger_( logger ),
   config_( config )
{
  logger_->log_info("CAbstractDriveMode", "(Constructor): Entering...");
  m_ProposedTranslation = 0.0;
  m_ProposedRotation = 0.0;
  m_DriveModeName = NavigatorInterface::MovingNotAllowed;

  // read m_cMaxTransDec and m_cMaxRotDec
  m_cMaxTransDec = 0.75*config_->get_float("/plugins/colli/QuadraticMotorInstruct/BASIC_TRANS_DEC");
  m_cMaxRotDec   = 0.75*config_->get_float("/plugins/colli/QuadraticMotorInstruct/BASIC_ROT_DEC");

  logger_->log_info("CAbstractDriveMode", "(Constructor): Exiting...");
}

/** Desctructor. */
inline
CAbstractDriveMode::~CAbstractDriveMode()
{
  logger_->log_info("CAbstractDriveMode", "(Destructor): Entering...");
  logger_->log_info("CAbstractDriveMode", "(Destructor): Exiting...");
}


/** Returns the proposed translation which was calculated previously in
 *  'Update()' which has to be implemented!
 * @return The proposed translation
 */
inline float
CAbstractDriveMode::GetProposedTranslation()
{
  return m_ProposedTranslation;
}

/** Returns the proposed rotation which was calculated previously in
 *  'Update()' which has to be implemented!
 * @return The proposed rotation
 */
inline float
CAbstractDriveMode::GetProposedRotation()
{
  return m_ProposedRotation;
}


/** Sets the current target.
 *  Has to be set before Update!
 * @param targetX The target x position
 * @param targetY The target y position
 * @param targetOri The target orientation
 */
inline void
CAbstractDriveMode::SetCurrentTarget( float targetX, float targetY, float targetOri )
{
  m_TargetX   = targetX;
  m_TargetY   = targetY;
  m_TargetOri = targetOri;
}

/** Sets the current robo position.
 *  Has to be set before Update!
 * @param roboX The robot x position
 * @param roboY The robot y position
 * @param roboOri The robot orientation
 */
inline void
CAbstractDriveMode::SetCurrentRoboPos( float roboX, float roboY, float roboOri )
{
  m_RoboX   = roboX;
  m_RoboY   = roboY;
  m_RoboOri = roboOri;
}

/** Sets the current robo speed.
 *  Has to be set before Update!
 * @param roboTrans The robot translation velocity
 * @param roboRot The robot rotation velocity
 */
inline void
CAbstractDriveMode::SetCurrentRoboSpeed( float roboTrans, float roboRot )
{
  m_RoboTrans = roboTrans;
  m_RoboRot   = roboRot;
}

/** Set the colli mode values for each drive mode.
 *  Has to be set before Update!
 * @param orient Orient at target after target position reached?
 * @param stop Stop at target position?
 */
inline void
CAbstractDriveMode::SetCurrentColliMode( bool orient, bool stop )
{
  m_OrientAtTarget = orient;
  m_StopAtTarget   = stop;
}


/** Set the local targetpoint found by the search.
 *  Has to be set before Update!
 * @param localTargetX The local target x position
 * @param localTargetY The local target y position
 */
inline void
CAbstractDriveMode::SetLocalTarget( float localTargetX, float localTargetY )
{
  m_LocalTargetX = localTargetX;
  m_LocalTargetY = localTargetY;
}

/** Set the local trajectory point found by the search.
 *  Has to be set before Update!
 * @param localTrajecX The local target x trajectory
 * @param localTrajecY The local target y trajectory
 */
inline void
CAbstractDriveMode::SetLocalTrajec( float localTrajecX, float localTrajecY )
{
  m_LocalTrajecX = localTrajecX;
  m_LocalTrajecY = localTrajecY;
}

/** Get the drive mode.
 *  Has to be set in the constructor of your drive mode!
 * @return The drive mode
 */
inline fawkes::NavigatorInterface::DriveMode
CAbstractDriveMode::GetDriveModeName()
{
  return m_DriveModeName;
}

/** Performs linear interpolation
 * @param x  x
 * @param x1 x1
 * @param x2 x2
 * @param y1 y1
 * @param y2 y2
 * @return interpolated value
 */
inline float
CAbstractDriveMode::LinInterpol( float x, float x1, float x2,
                float y1, float y2 )
{
  return ( ((x-x1)*(y2-y1))/(x2-x1) + y1 );
}

/** Get velocity that guarantees a stop for a given distance
 * @param distance The distance to stop at
 * @param current_trans Robot's current translation velocity
 * @param desired_trans Robot's currently desired translation velocity
 * @return Proposed translation velocity to stop at given distance
 */
inline float
CAbstractDriveMode::GuaranteeTransStop( float distance,
                 float current_trans,
                 float desired_trans )
{
  distance = fabs( distance );
  current_trans = fabs( current_trans );


  if ( distance < 0.05 )
    return 0.0;

  if ( current_trans < 0.05 )
    return desired_trans;

  int time_needed_to_distance = (int)( distance / (current_trans/10.0) );
  int time_needed_to_stop = (int)( desired_trans / m_cMaxTransDec );

  if( time_needed_to_stop >= time_needed_to_distance ) {
    float value = std::max( 0.0, current_trans - (1.0 * m_cMaxTransDec) );
    return value;
  } else {
    float value = std::min( current_trans + m_cMaxTransDec, desired_trans );
    return value;
  }
}

} // end namespace fawkes

#endif
