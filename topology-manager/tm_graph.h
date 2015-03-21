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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/foreach.hpp>

#include <bitvector.h>

#include <blackadder_defs.h>

struct network;
struct node;
struct connection;

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

/* blackadder network */
struct network
{
  std::map<std::string, node_ptr> nodes;

  node_ptr rv_node;
  node_ptr tm_node;
};

/* a blackadder network node */
struct node
{
  /* assigned through parsing the configuration file */

  std::string label;				// parsed
  bool is_rv;					// parsed
  bool is_tm;					// parsed

  /* internally connections are unidirectional - they are indexed by the destination node label */
  std::multimap<std::string, connection_ptr> connections;

  /* used internally */
  bitvector internal_link_id;
};

/* a unidirectional blackadder network connection */
struct connection
{
  std::string src_label;	// assigned through parsing the configuration file
  std::string dst_label;	// assigned through parsing the configuration file

  bitvector link_id;		// used internally
};

typedef boost::shared_ptr<bitvector> lipsin_id_ptr;

/* these must be indexed */
struct forwarding_entry
{
  /* source node label */
  std::string source;

  /* destination node label */
  std::string destination;

  /* number of hops */
  unsigned int no_hops;

  /* lipsin identifier */
  lipsin_id_ptr lipsin_ptr;
};
typedef boost::shared_ptr<forwarding_entry> forwarding_entry_ptr;

typedef std::map<std::string, forwarding_entry_ptr> per_dst_fw_entry;
typedef std::pair<std::string, forwarding_entry_ptr> per_dst_fw_pair;

typedef boost::shared_ptr<per_dst_fw_entry> per_dst_fw_entry_ptr;

typedef std::map<std::string, per_dst_fw_entry_ptr> per_node_fib_index;
typedef std::pair<std::string, per_dst_fw_entry_ptr> per_node_fib_pair;

/* global routing table is pre-calculated on initialisation */
extern std::string topology_manager_label;
extern per_node_fib_index fib;

/* free function that parses the configuration file using boost property_tree library */
void
parse_configuration (boost::property_tree::ptree &pt, const std::string &filename);

void
load_network (network_ptr net_ptr, const std::string &filename);

/* print network information */
void
print_network (network_ptr net_ptr);

void
load_node (network_ptr net_ptr, node_ptr n_ptr, const boost::property_tree::ptree &pt);

void
load_connection (connection_ptr c_ptr, const boost::property_tree::ptree &pt);

void
read_topology (network_graph_ptr net_graph_ptr, std::string filename);

/* use network configuration to produce a boost adjacency list */
void
create_graph (network_graph_ptr net_graph_ptr, network_ptr net_ptr);

void
build_forwarding_base (network_graph_ptr net_graph_ptr);

void
print_forwarding_base (network_graph_ptr net_graph_ptr);

void
match_pubs_subs (std::set<std::string> &publishers, std::set<std::string> &subscribers, std::map<std::string, boost::shared_ptr<bitvector> > &result);

void
shortest_path (std::string &source, std::string &destination, boost::shared_ptr<bitvector> lipsin_ptr);
#endif
