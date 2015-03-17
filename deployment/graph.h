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

#ifndef GRAPH_H_
#define GRAPH_H_

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/foreach.hpp>

#include <map>

#include "params.h"
#include "bitvector.h"
#include "network.h"

/* forward declarations used here and in network.h */

struct network;
struct node;
struct connection;
struct ns3_application;

/* type definitions for shared_ptr for the structures above*/
typedef boost::shared_ptr<network> network_ptr;
typedef boost::shared_ptr<node> node_ptr;
typedef boost::shared_ptr<connection> connection_ptr;
typedef boost::shared_ptr<ns3_application> ns3_application_ptr;

/* type definition for boost adjacency list */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, node_ptr, connection_ptr, network_ptr> network_graph;
typedef boost::shared_ptr<network_graph> network_graph_ptr;

/* type definitions for boost graph vertex and edge descriptors */
typedef boost::graph_traits<network_graph>::vertex_descriptor vertex;
typedef boost::graph_traits<network_graph>::edge_descriptor edge;

typedef boost::filesystem::path path;

/* use network configuration to produce a boost adjacency list */
void
create_graph (network_graph_ptr net_graph_ptr, network_ptr net_ptr);

/* assign Link Identifiers and internal link identifiers */
void
assign_link_ids (network_graph_ptr net_graph_ptr);

/* calculate and assign forwarding identifiers to a target node for all nodes in the network */
void
calculate_forwarding_ids (network_graph_ptr net_graph_ptr);

/* ssh remote nodes to discover MAC addresses for Ethernet interfaces specified in the configuration file */
void
discover_mac_addresses (network_graph_ptr net_graph_ptr);

/* copy Click configuration files remotely to machines */
void
write_click_conf (network_graph_ptr net_graph_ptr, std::string &output_folder);

/* writes configuration (a light boost property tree) for the topology manager */
void
write_tm_conf (network_graph_ptr net_graph_ptr, std::string &output_folder);

void
scp_click_conf (network_graph_ptr net_graph_ptr, std::string &output_folder);

void
scp_tm_conf (network_graph_ptr net_graph_ptr, std::string &output_folder);

void
start_click (network_graph_ptr net_graph_ptr);

void
start_tm (network_graph_ptr net_graph_ptr);

#endif /* GRAPH_H_ */
