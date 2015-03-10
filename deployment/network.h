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

#ifndef NETWORK_HPP
#define	NETWORK_HPP

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

#include <map>

#include "bitvector.h"

struct network;
struct node;
struct connection;
struct ns3_application;

/* type definitions for shared_ptr for the above structs */
typedef boost::shared_ptr<network> network_ptr;
typedef boost::shared_ptr<node> node_ptr;
typedef boost::shared_ptr<connection> connection_ptr;
typedef boost::shared_ptr<ns3_application> ns3_application_ptr;

/* type definition for boost adjacency list */
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, node_ptr, connection_ptr, network_ptr> network_graph;

/* type definitions for pairs to use with BOOST_FOREACH macro */
typedef std::pair<std::string, connection_ptr> connection_map_pair_t;
typedef std::pair<std::string, node_ptr> node_map_pair_t;

/* type definitions for boost graph vertex and edge descriptors */
typedef boost::graph_traits<network_graph>::vertex_descriptor vertex;
typedef boost::graph_traits<network_graph>::edge_descriptor edge;

/* type definitions for vertex and edge iterators */
typedef boost::graph_traits<network_graph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<network_graph>::out_edge_iterator out_edge_iter;

typedef boost::filesystem::path path;

/* free function that parses the configuration file using boost property_tree library */
void
parse_configuration (boost::property_tree::ptree &pt, const std::string &filename, const std::string &format);

/* blackadder network */
struct network
{
  /* assigned through parsing the configuration file */
  int info_id_len;
  int link_id_len;
  bool is_simulation;

  std::string user; 			// can be specified globally
  bool sudo;				// can be specified globally
  std::string click_home;		// can be specified globally
  std::string conf_home;		// can be specified globally
  std::string running_mode;		// can be specified globally
  std::string operating_system;		// can be specified globally

  std::map<std::string, node_ptr> nodes;

  node_ptr rv_node;
  node_ptr tm_node;

  /* used internally */

  /* maps node labels to vertex descriptors in the boost graph */
  std::map<std::string, vertex> vertices_map;

  /* a boost bidirectional, directed graph to use throughout blackadder deployment */
  network_graph net_graph;

  /* methods for internal structures*/
  void
  load (const std::string &filename, const std::string &format);

  /* calculate next link identifier to be assigned */
  void
  calculate_lid (std::map<std::string, bitvector>& link_identifiers, int index);

  /* assign Link Identifiers and internal link identifiers */
  void
  assign_link_ids ();

  /* ssh remote nodes to discover MAC addresses for Ethernet interfaces specified in the configuration file */
  void
  discover_mac_addresses ();

  /* copy Click configuration files remotely to machines */
  void
  write_click_conf ();

  void
  scp_click_conf ();

  void
  start_click ();

  void
  scp_tm_conf (std::string tm_conf);

  void
  start_tm ();

  /* print network information */
  void
  print ();

  /* methods for boost graph representation of the network */

  /* use network configuration to produce a boost adjacency list */
  void
  create_graph ();

  void
  calculate_forwarding_id (vertex src_v, vertex dst_v, std::vector<vertex> &predecessor_vector, bitvector &lipsin);

  /* calculate forwarding identifiers to rv and tm nodes for all nodes in the network */
  void
  calculate_forwarding_ids ();

  /* print the network graph */
  void
  print_graph ();
};

/* network node */
struct node
{
  /* assigned through parsing the configuration file */

  std::string label;			// parsed
  std::string testbed_ip;		// parsed
  bool is_rv;			// parsed
  bool is_tm;			// parsed

  std::string user;			// parsed or provided globally for the network
  bool sudo;			// parsed or provided globally for the network
  std::string click_home;		// parsed, inferred based on the username (/home/user_name/click) or provided globally for the network
  std::string conf_home;		// parsed, inferred based on the username (/home/user_name/conf) or provided globally for the network

  std::string running_mode;		// parsed or provided globally for the network (default: user)
  std::string operating_system;	// parsed or provided globally for the network (default: Linux)

  /* internally connections are unidirectional - they are indexed by the destination node label */
  std::multimap<std::string, connection_ptr> connections;

  /* if is_simulation == true */
  std::vector<ns3_application_ptr> ns3_applications;
  int device_offset;

  /* used internally */
  bitvector internal_link_id;

  /* lipsin identifier to forward to rv node */
  bitvector lipsin_rv;

  /* lipsin identifier to forward to tm node */
  bitvector lipsin_tm;

  void
  load (const boost::property_tree::ptree &pt, struct network &network);
};

/* network connection */
struct connection
{
  /* assigned through parsing the configuration file */

  std::string overlay_mode;
  bool is_bidirectional;

  std::string src_label;
  std::string dst_label;

  std::string src_if; /* e.g. tap0 or eth1 */
  std::string dst_if; /* e.g. tap0 or eth1 */

  std::string src_ip; /* an IP address - i will not resolve mac addresses in this case */
  std::string dst_ip; /* an IP address - i will not resolve mac addresses in this case */

  std::string src_mac;
  std::string dst_mac;

  /* used internally */
  bitvector link_id;

  void
  load (const boost::property_tree::ptree &pt);
  void
  reverse (connection_ptr reverse_ptr);
};

/* an ns-3 application */
struct ns3_application
{
  std::string node_label;

  void
  load (const boost::property_tree::ptree &pt);
};

#endif
