
/***************************************************************************
 *  topological_map_node.h - Topological graph node
 *
 *  Created: Fri Sep 21 16:01:26 2012
 *  Copyright  2012  Tim Niemueller [www.niemueller.de]
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

#ifndef __UTILS_GRAPH_TOPOLOGICAL_MAP_NODE_H_
#define __UTILS_GRAPH_TOPOLOGICAL_MAP_NODE_H_

#include <utils/misc/string_conversions.h>

#include <map>
#include <vector>
#include <string>

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class TopologicalMapNode {
 public:
  TopologicalMapNode();

  TopologicalMapNode(std::string name, float x, float y,
                     std::map<std::string, std::string> properties);

  TopologicalMapNode(std::string name, float x, float y);

  const std::string &  name() const
  { return name_; }

  float x() const
  { return x_; }

  float y() const
  { return y_; }

  void set_x(float x);
  void set_y(float y);
  void set_name(std::string name);

  std::map<std::string, std::string> &  properties()
  { return properties_; }

  bool has_property(std::string property)
  { return properties_.find(property) != properties_.end(); }

  bool is_valid() const
  { return name_ != ""; }

  void set_property(std::string property, std::string value);
  void set_property(std::string property, float value);
  void set_property(std::string property, int value);
  void set_property(std::string property, bool value);

  std::string property(std::string prop);

  float property_as_float(std::string prop)
  { return StringConversions::to_float(property(prop)); }

  int property_as_int(std::string prop)
  { return StringConversions::to_int(property(prop)); }

  bool property_as_bool(std::string prop)
  { return StringConversions::to_bool(property(prop)); }

  /** Check nodes for equality.
   * Nodes are equal if they have the same name.
   * @param n node to compare with
   * @return true if the node is the same as this one, false otherwise
   */
  bool operator==(const TopologicalMapNode &n) const
  { return name_ == n.name_; }

  void set_reachable_nodes(std::vector<std::string> reachable_nodes);
  const std::vector<std::string> &  reachable_nodes() const
  { return reachable_nodes_; }
    
 private:
  std::string name_;
  float       x_;
  float       y_;
  std::map<std::string, std::string> properties_;
  std::vector<std::string> reachable_nodes_;
};


} // end of namespace fawkes

#endif
