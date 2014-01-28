
/***************************************************************************
 *  tf.cpp - Transform aspect for Fawkes
 *
 *  Created: Tue Oct 25 21:35:14 2011
 *  Copyright  2006-2011  Tim Niemueller [www.niemueller.de]
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

#include <aspect/tf.h>
#include <tf/transform_listener.h>

#include <cstring>
#include <cstdlib>
#include <core/threading/thread_initializer.h>

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** @class TransformAspect <aspect/tf.h>
 * Thread aspect to access the transform system.

 * Give this aspect to your thread to gain access to the transform
 * library.  Depending on the parameters to the ctor only the listener
 * or additionaly the publisher is created.
 * It is guaranteed that if used properly from within plugins that the
 * blackboard member has been initialized properly.
 * @ingroup Aspects
 * @author Tim Niemueller
 */


/** @var tf::TransformListener *  TransformAspect::tf_listener
 * This is the transform listener which saves transforms published by
 * other threads in the system.
 */

/** @var tf::TransformPublisher *  TransformAspect::tf_publisher
 * This is the transform publisher which can be used to publish
 * transforms via the blackboard. It is only created if the constructor
 * taking the blackboard interface ID parameter is used!
 */

/** Constructor.
 * @param mode mode of operation
 * @param tf_bb_iface_id interface ID to be used for the transform
 * publisher. Note that this will be prefixed with "TF ".
 */
TransformAspect::TransformAspect(Mode mode, const char *tf_bb_iface_id)
  : __tf_aspect_mode(mode)
{
  add_aspect("TransformAspect");
  if (((mode == ONLY_PUBLISHER) || (mode == BOTH) || (mode == BOTH_DEFER_PUBLISHER))
       && tf_bb_iface_id)
    {
    __tf_aspect_bb_iface_id = strdup(tf_bb_iface_id);
  } else {
    __tf_aspect_bb_iface_id = 0;
  }
  __tf_aspect_blackboard = 0;
}


/** Virtual empty destructor. */
TransformAspect::~TransformAspect()
{
  if (__tf_aspect_bb_iface_id)  free(__tf_aspect_bb_iface_id);
}


/** Init transform aspect.
 * This creates the listener and potentially publisher.
 * @param blackboard blackboard used to create listener and/or publisher.
 * @param transformer system-wide shared transformer to pass to threads
 */
void
TransformAspect::init_TransformAspect(BlackBoard *blackboard, tf::Transformer *transformer)
{
  if (((__tf_aspect_mode == ONLY_PUBLISHER) || (__tf_aspect_mode == BOTH)) &&
      (__tf_aspect_bb_iface_id == NULL))
  {
    throw CannotInitializeThreadException("TransformAspect was initialized "
                                          "in mode %s but BB interface ID"
                                          "is not set",
                                          (__tf_aspect_mode == BOTH) ? "BOTH"
                                          : "ONLY_PUBLISHER");
  }

  __tf_aspect_blackboard = blackboard;

  if (( (__tf_aspect_mode == DEFER_PUBLISHER) || (__tf_aspect_mode == BOTH_DEFER_PUBLISHER))
      && (__tf_aspect_bb_iface_id == 0))
  {
    throw CannotInitializeThreadException("TransformAspect in %s mode "
					  "requires a valid blackboard interface ID",
					  __tf_aspect_mode == DEFER_PUBLISHER
					  ? "DEFER_PUBLISHER" : "BOTH_DEFER_PUBLISHER" );
  }

  if ((__tf_aspect_mode == ONLY_LISTENER) || (__tf_aspect_mode == BOTH) ||
      (__tf_aspect_mode == BOTH_DEFER_PUBLISHER))
  {
    __tf_aspect_own_listener = false;
    tf_listener = transformer;
  } else {
    __tf_aspect_own_listener = true;
    tf_listener = new tf::TransformListener(NULL);
  }

  if ((__tf_aspect_mode == ONLY_PUBLISHER) || (__tf_aspect_mode == BOTH)) {
    tf_publisher =
      new tf::TransformPublisher(blackboard, __tf_aspect_bb_iface_id);
  } else {
    tf_publisher = new tf::TransformPublisher(NULL, NULL);
  }
}


/** Late enabling of publisher.

 * If and only if the TransformAspect has been initialized in
 * DEFER_PUBLISHER or BOTH_DEFER_PUBLISHER mode the transform
 * publisher can be enabled using this method. It will create a new
 * transform publisher with the interface ID given as constructor
 * parameter.
 *
 * This method is intended to be used if it is unclear at construction
 * time whether the publisher will be needed or not.
 * @exception Exception thrown if the TransformAspect is not initialized in
 * DEFER_PUBLISHER or BOTH_DEFER_PUBLISHER mode.
 */
void
TransformAspect::tf_enable_publisher()
{
  if ((__tf_aspect_mode != DEFER_PUBLISHER) && (__tf_aspect_mode != BOTH_DEFER_PUBLISHER)) {
    throw Exception("Publisher can only be enabled later in (BOTH_)DEFER_PUBLISHER mode");
  }
  delete tf_publisher;
  tf_publisher =
    new tf::TransformPublisher(__tf_aspect_blackboard, __tf_aspect_bb_iface_id);
}

/** Finalize transform aspect.
 * This deletes the transform listener and publisher.
 */
void
TransformAspect::finalize_TransformAspect()
{
  if (__tf_aspect_own_listener) {
    delete tf_listener;
  }
  delete tf_publisher;
  tf_listener = 0;
  tf_publisher = 0;
}

} // end namespace fawkes
