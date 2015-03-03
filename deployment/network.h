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

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/program_options.hpp>
#include <boost/graph/graphml.hpp>

#include <boost/smart_ptr.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/property_map/property_map.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/foreach.hpp>

#include "bitvector.h"

using namespace std;

struct network;
struct node;
struct link;

void
parse_configuration (boost::property_tree::ptree &pt, const string &filename, const string &format);

struct network
{
  /* assigned through parsing the configuration file */
  int info_id_len;
  int link_id_len;
  bool is_simulation;

  string user; 			// can be specified globally
  bool sudo;			// can be specified globally
  string click_home;		// can be specified globally
  string conf_home;		// can be specified globally
  string running_mode;		// can be specified globally
  string operating_system;	// can be specified globally

  map<string, struct node> nodes;

  node *rv_node;
  node *tm_node;

  /* used internally */

  void
  load (const string &filename, const string &format);
  /* assign Link Identifiers and internal link identifiers */
  void
  assign_link_ids ();
  void
  calculate_lid (map<string, bitvector>& link_identifiers, int index);
  /* print network information */
  void
  print ();

};

struct node
{
  /* assigned through parsing the configuration file */

  string label;			// parsed
  string testbed_ip;		// parsed
  bool is_rv;			// parsed
  bool is_tm;			// parsed

  string user;			// parsed or provided globally for the network
  bool sudo;			// parsed or provided globally for the network
  string click_home;		// parsed, inferred based on the username (/home/user_name/click) or provided globally for the network
  string conf_home;		// parsed, inferred based on the username (/home/user_name/conf) or provided globally for the network

  string running_mode;		// parsed or provided globally for the network (default: User)
  string operating_system;	// parsed or provided globally for the network (default: Linux)

  /* internally connections are unidirectional - they are indexed by the destination node label */
  map<string, struct connection> connections;

  vector<struct ns3_application> ns3_applications;

  /* used internally */
  bitvector internal_link_id;

  void
  load (const boost::property_tree::ptree &pt, struct network &network);
};

struct connection
{
  /* assigned through parsing the configuration file */

  string overlay_mode;
  bool is_bidirectional;

  string src_label; 		//read from configuration file
  string dst_label; 		//read from configuration file

  string src_if; 		//read from configuration file /*e.g. tap0 or eth1*/
  string dst_if; 		//read from configuration file /*e.g. tap0 or eth1*/

  string src_ip; 		//read from configuration file /*an IP address - i will not resolve mac addresses in this case*/
  string dst_ip; 		//read from configuration file /*an IP address - i will not resolve mac addresses in this case*/

  string src_mac; 		//will be retrieved using ssh
  string dst_mac; 		//will be retrieved using ssh

  /* used internally */
  bitvector link_id;

  void
  load (const boost::property_tree::ptree &pt);
  void
  reverse (connection &connection);
};

/* an ns-3 application */
struct ns3_application
{
  string node_label;

  void
  load (const boost::property_tree::ptree &pt);
};

#endif
