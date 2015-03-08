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

#ifndef DEPLOY_H_
#define DEPLOY_H_

using namespace std;

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/program_options.hpp>

#include <boost/foreach.hpp>

#include "network.h"

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, node_ptr, connection_ptr, network> network_graph;

typedef pair<string, connection_ptr> connection_map_pair_t;
typedef pair<string, node_ptr> node_map_pair_t;

typedef boost::graph_traits<network_graph>::vertex_descriptor vertex;
typedef boost::graph_traits<network_graph>::edge_descriptor edge;
typedef boost::graph_traits<network_graph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<network_graph>::out_edge_iterator out_edge_iter;

map<string, vertex> vertices_map;

void
create_graph (network &network, network_graph &network_graph);

void
calculate_forwarding_id (vertex src_v, vertex dst_v, network_graph &net_graph);

void
calculate_forwarding_ids (network &network, network_graph &net_graph);

void
print_graph (network_graph &net_graph);

#endif /* DEPLOY_H_ */
