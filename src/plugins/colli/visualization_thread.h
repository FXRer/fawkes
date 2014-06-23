/***************************************************************************
 *  visualization_thread.h - Visualization colli
 *
 *  Created: Fri Oct 18 15:16:23 2013
 *  Copyright  2013  Bahram Maleki-Fard
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

#ifndef __PLUGINS_COLLI_VISUALIZATION_THREAD_H_
#define __PLUGINS_COLLI_VISUALIZATION_THREAD_H_

#ifdef HAVE_VISUAL_DEBUGGING

#include <core/threading/thread.h>
#include <core/threading/mutex.h>
#include <aspect/tf.h>
#include <aspect/configurable.h>
#include <plugins/ros/aspect/ros.h>

#include <vector>

namespace ros {
  class Publisher;
}

namespace fawkes {
  class CLaserOccupancyGrid;
  class CSearch;
  class Laser;
  typedef struct point_struct point_t;
}
#endif

class ColliVisualizationThread
#ifndef HAVE_VISUAL_DEBUGGING
{
#else
: public fawkes::Thread,
  public fawkes::TransformAspect,
  public fawkes::ConfigurableAspect,
  public fawkes::ROSAspect
{
 public:
  ColliVisualizationThread();

  virtual void init();
  virtual void loop();
  virtual void finalize();

  virtual void setup(fawkes::CLaserOccupancyGrid* occ_grid,
                     fawkes::CSearch* search,
                     fawkes::Laser* laser);

 private:
  fawkes::Mutex mutex_;

  fawkes::CLaserOccupancyGrid *occ_grid_;
  fawkes::CSearch             *search_;
  fawkes::Laser               *laser_;

  ros::Publisher *pub_laser_;
  ros::Publisher *pub_cells_occ_;
  ros::Publisher *pub_cells_near_;
  ros::Publisher *pub_cells_mid_;
  ros::Publisher *pub_cells_far_;
  ros::Publisher *pub_cells_free_;
  ros::Publisher *pub_search_path_;

  std::vector<fawkes::point_t> cells_occ_;
  std::vector<fawkes::point_t> cells_near_;
  std::vector<fawkes::point_t> cells_mid_;
  std::vector<fawkes::point_t> cells_far_;
  std::vector<fawkes::point_t> cells_free_;

#endif
};

#endif
