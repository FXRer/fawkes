
/***************************************************************************
 *  acquisition_thread.h - Thread that retrieves the joystick data
 *
 *  Created: Sat Nov 22 18:10:35 2008
 *  Copyright  2006-2008  Tim Niemueller [www.niemueller.de]
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

#ifndef __PLUGINS_JOYSTICK_ACQUISITION_THREAD_H_
#define __PLUGINS_JOYSTICK_ACQUISITION_THREAD_H_

#include <core/threading/thread.h>
#include <aspect/logging.h>
#include <aspect/configurable.h>

#include <utils/math/types.h>

#include <string>
#include <vector>

namespace fawkes {
  class Mutex;
}

class JoystickBlackBoardHandler
{
 public:
  virtual ~JoystickBlackBoardHandler();
  virtual void joystick_changed(unsigned int pressed_buttons,
				float *axis_x_values, float *axis_y_values) = 0;
  virtual void joystick_plugged(char num_axes, char num_buttons) = 0;
  virtual void joystick_unplugged() = 0;
};

class JoystickAcquisitionThread
: public fawkes::Thread,
  public fawkes::LoggingAspect,
  public fawkes::ConfigurableAspect
{
 public:
  JoystickAcquisitionThread();
  JoystickAcquisitionThread(const char *device_file,
			    JoystickBlackBoardHandler *handler,
			    fawkes::Logger *logger);

  virtual void init();
  virtual void finalize();
  virtual void loop();

  bool lock_if_new_data();
  void unlock();

  char               num_axes() const;
  char               num_buttons() const;
  const char *       joystick_name() const;
  unsigned int       pressed_buttons() const;
  float *            axis_x_values();
  float *            axis_y_values();

 /** Stub to see name in backtrace for easier debugging. @see Thread::run() */
 protected: virtual void run() { Thread::run(); }

 private:
  void init(std::string device_file);
  void open_joystick();

 private:
  std::string __cfg_device_file;

  int  __fd;
  bool __connected;
  unsigned int __axis_array_size;
  char __num_axes;
  char __num_buttons;
  char __joystick_name[128];

  bool            __new_data;
  fawkes::Mutex  *__data_mutex;

  unsigned int    __pressed_buttons;
  float          *__axis_x_values;
  float          *__axis_y_values;

  JoystickBlackBoardHandler *__bbhandler;
};


#endif
