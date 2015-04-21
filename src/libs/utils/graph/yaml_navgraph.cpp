
/***************************************************************************
 *  yaml_navgraph.cpp - Nav graph stored in a YAML file
 *
 *  Created: Fri Sep 21 18:37:16 2012
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

#include <utils/graph/yaml_navgraph.h>
#include <utils/graph/topological_map_graph.h>
#include <core/exception.h>

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

/** Read topological map node from YAML iterator.
 * @param n iterator to node representing a topological map graph node
 * @param node node to fill
 */
static void
operator >> (const YAML::Node& n, TopologicalMapNode &node) {
#ifdef HAVE_YAMLCPP_0_5
  const std::string name = n["name"].as<std::string>();
#else
  std::string name;
  n["name"] >> name;
#endif

#ifdef HAVE_OLD_YAMLCPP
  if (n.GetType() != YAML::CT_MAP) {
#else
  if (n.Type() != YAML::NodeType::Map) {
#endif
    throw Exception("Node %s is not a map!?", name.c_str());
  }

  try {
    if (n["pos"].size() != 2) {
      throw Exception("Invalid position for node %s, "
                      "must be list of [x,y] coordinates", name.c_str());
    }
    float x, y;
#ifdef HAVE_YAMLCPP_0_5
    x = n["pos"][0].as<float>();
    y = n["pos"][1].as<float>();
#else
    n["pos"][0] >> x;
    n["pos"][1] >> y;
#endif

    node.set_x(x);
    node.set_y(y);
  } catch (YAML::Exception &e) {
    throw fawkes::Exception("Failed to parse node: %s", e.what());
  }

#ifdef HAVE_OLD_YAMLCPP
  if (n.GetTag() == "tag:fawkesrobotics.org,navgraph/unconnected") {
#else
  if (n.Tag() == "tag:fawkesrobotics.org,navgraph/unconnected") {
#endif
    node.set_unconnected(true);
  }

  bool has_properties = true;
  try {
#ifdef HAVE_YAMLCPP_0_5
    has_properties = n["properties"].IsDefined();
#else
    has_properties = (n.FindValue("properties") != NULL);
#endif
  } catch (YAML::Exception &e) {
    has_properties = false;
  }

  if (has_properties) {
    try {
      const YAML::Node &props = n["properties"];
      if (props.Type() != YAML::NodeType::Sequence) {
	throw Exception("Properties must be a list");
      }

      std::map<std::string, std::string> properties;

#ifdef HAVE_YAMLCPP_0_5
      YAML::const_iterator p;
#else
      YAML::Iterator p;
#endif
      for (p = props.begin(); p != props.end(); ++p) {
#ifdef HAVE_OLD_YAMLCPP
	if (p->GetType() == YAML::CT_SCALAR) {
#else
	if (p->Type() == YAML::NodeType::Scalar) {
#endif
#ifdef HAVE_YAMLCPP_0_5
	  std::string key = p->as<std::string>();
#else
	  std::string key;
	  *p >> key;
#endif
	  node.set_property(key, "true");
#ifdef HAVE_OLD_YAMLCPP
	} else if (p->GetType() == YAML::CT_MAP) {
#else
	} else if (p->Type() == YAML::NodeType::Map) {
#endif
#ifdef HAVE_YAMLCPP_0_5
	  for (YAML::const_iterator i = p->begin(); i != p->end(); ++i) {
	    std::string key   = i->first.as<std::string>();
	    std::string value = i->second.as<std::string>();
#else
	  for (YAML::Iterator i = p->begin(); i != p->end(); ++i) {
	    std::string key, value;
	    i.first() >> key;
	    i.second() >> value;
#endif
	    node.set_property(key, value);
	  }
	} else {
	  throw Exception("Invalid property for node '%s'", name.c_str());
	}
      }    
    } catch (YAML::Exception &e) {
      throw Exception("Failed to read propery of %s: %s",
		      name.c_str(), e.what());
    } // ignored
  }

  node.set_name(name);
}

/** Read topological map edge from YAML iterator.
 * @param n iterator to node representing a topological map graph edge
 * @param edge edge to fill
 */
static void
operator >> (const YAML::Node& n, TopologicalMapEdge &edge) {
#ifdef HAVE_OLD_YAMLCPP
  if (n.GetType() != YAML::CT_SEQUENCE || n.size() != 2) {
#else
  if (n.Type() != YAML::NodeType::Sequence || n.size() != 2) {
#endif
    throw Exception("Invalid edge");
  }
  std::string from, to;
#ifdef HAVE_YAMLCPP_0_5
  from = n[0].as<std::string>();
  to   = n[1].as<std::string>();
#else
  n[0] >> from;
  n[1] >> to;
#endif

  edge.set_from(from);
  edge.set_to(to);

#ifdef HAVE_OLD_YAMLCPP
  if (n.GetTag() == "tag:fawkesrobotics.org,navgraph/dir") {
#else
  if (n.Tag() == "tag:fawkesrobotics.org,navgraph/dir") {
#endif
    edge.set_directed(true);
  }
}



/** Load topological map graph stored in RCSoft format.
 * @param filename path to the file to read
 * @return topological map graph read from file
 * @exception Exception thrown on any error to read the graph file
 */
TopologicalMapGraph *
load_yaml_navgraph(std::string filename)
{
  //try to fix use of relative paths
  if (filename[0] != '/') {
    filename = std::string(CONFDIR) + "/" + filename;
  }

  YAML::Node doc;
#ifdef HAVE_YAMLCPP_0_5
  if (! (doc = YAML::LoadFile(filename))) {
#else
  std::ifstream fin(filename.c_str());
  YAML::Parser parser(fin);
  if (! parser.GetNextDocument(doc)) {
#endif
    throw fawkes::Exception("Failed to read YAML file %s", filename.c_str());
  }

#ifdef HAVE_YAMLCPP_0_5
  std::string graph_name = doc["graph-name"].as<std::string>();
#else
  std::string graph_name;
  doc["graph-name"] >> graph_name;
#endif

  TopologicalMapGraph *graph = new TopologicalMapGraph(graph_name);

  const YAML::Node &nodes = doc["nodes"];
#ifdef HAVE_YAMLCPP_0_5
  for (YAML::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
#else
  for (YAML::Iterator n = nodes.begin(); n != nodes.end(); ++n) {
#endif
    TopologicalMapNode node;
    *n >> node;
    graph->add_node(node);
  }

  const YAML::Node &edges = doc["connections"];
#ifdef HAVE_YAMLCPP_0_5
  for (YAML::const_iterator e = edges.begin(); e != edges.end(); ++e) {
#else
  for (YAML::Iterator e = edges.begin(); e != edges.end(); ++e) {
#endif
    TopologicalMapEdge edge;
    *e >> edge;
    graph->add_edge(edge);
  }

  graph->calc_reachability();
  return graph;
}

} // end of namespace fawkes