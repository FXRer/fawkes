/***************************************************************************
 *  kalman_1d.h - Kalman filter (one dimensional)
 *
 *  Created: Mon Nov 10 2008
 *  Copyright  2008  Bahram Maleki-Fard
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

#ifndef __UTILS_KALMAN_KALMAN_1D_H_
#define __UTILS_KALMAN_KALMAN_1D_H_

namespace fawkes  {

class KalmanFilter1D
{
public:
	KalmanFilter1D(float noise_x = 1.0, float noise_z = 1.0, float mu = 0.0,
	               float sig = 1.0);
	~KalmanFilter1D();

	void filter(float observe);
	void filter(float observe, float& mu, float& sig);
	float predict() const;
	float predict(float vel) const;
	float predict(float vel, int steps, float noise_z) const;
	float predict(float mu, float vel, int steps, float noise_z) const;

private:
	float __noise_x;	/**< transition noise */
	float __noise_z;	/**< "sigma_z", sensor noise */
	float __mu;		/**< mean "mu" */
	float __sig;	/**< "sigma_0". sigma ~ standard deviation */
};
} // end namespace fawkes

#endif
