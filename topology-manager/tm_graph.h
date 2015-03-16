/*
 * Copyright (C) 2015  George Parisis
 * All rights reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef TM_IGRAPH_H
#define TM_IGRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graphviz.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/* blackadder network */
struct network
{
  std::string running_mode;

  std::string rv_node_label;
  std::string tm_node_label;
};

/* a blackadder network node */
struct node
{
  std::string label;
  std::string internal_link_id;
};

/* a unidirectional blackadder network connection */
struct connection
{
  std::string link_id;
};

/* type definitions for shared_ptr for the structures above*/
typedef boost::shared_ptr<network> network_ptr;
typedef boost::shared_ptr<node> node_ptr;
typedef boost::shared_ptr<connection> connection_ptr;

/* type definition for boost adjacency list */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, node_ptr, connection_ptr, network_ptr> network_graph;
typedef boost::shared_ptr<network_graph> network_graph_ptr;

/* type definitions for boost graph vertex and edge descriptors */
typedef boost::graph_traits<network_graph>::vertex_descriptor vertex;
typedef boost::graph_traits<network_graph>::edge_descriptor edge;

void
read_topology(network_graph_ptr net_graph_ptr, std::string filename);

#endif
