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

#include "graph.h"

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

/* blackadder network */
struct network
{
  int info_id_len;			// assigned through parsing the configuration file
  int link_id_len;			// assigned through parsing the configuration file
  bool is_simulation;			// assigned through parsing the configuration file

  std::string user; 			// can be specified globally
  bool sudo;				// can be specified globally
  std::string click_home;		// can be specified globally
  std::string conf_home;		// can be specified globally
  std::string running_mode;		// can be specified globally
  std::string operating_system;		// can be specified globally

  std::map<std::string, node_ptr> nodes;

  node_ptr rv_node;
  node_ptr tm_node;
};

/* a blackadder network node */
struct node
{
  /* assigned through parsing the configuration file */

  std::string label;			// parsed
  std::string testbed_ip;		// parsed
  bool is_rv;				// parsed
  bool is_tm;				// parsed

  std::string user;			// parsed or provided globally for the network
  bool sudo;				// parsed or provided globally for the network
  std::string click_home;		// parsed, inferred based on the username (/home/user_name/click) or provided globally for the network
  std::string conf_home;		// parsed, inferred based on the username (/home/user_name/conf) or provided globally for the network

  std::string running_mode;		// parsed or provided globally for the network (default: user)
  std::string operating_system;		// parsed or provided globally for the network (default: Linux)

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
};

/* a unidirectional blackadder network connection */
struct connection
{
  std::string overlay_mode;	// assigned through parsing the configuration file
  bool is_bidirectional;	// assigned through parsing the configuration file

  std::string src_label;	// assigned through parsing the configuration file
  std::string dst_label;	// assigned through parsing the configuration file

  std::string src_if;  		// e.g. tap0 or eth1
  std::string dst_if;  		// e.g. tap0 or eth1

  std::string src_ip;  		// an IP address - I will not resolve mac addresses in this case
  std::string dst_ip;  		// an IP address - I will not resolve mac addresses in this case

  std::string src_mac; 		// I will not resolve mac addresses in this case
  std::string dst_mac; 		// I will not resolve mac addresses in this case

  bitvector link_id;		// used internally
};

/* a simulated blackadder application */
struct ns3_application
{
  std::string node_label;
};

/* free function that parses the configuration file using boost property_tree library */
void
parse_configuration (boost::property_tree::ptree &pt, const std::string &filename, const std::string &format);

void
load_network (network_ptr net_ptr, const std::string &filename, const std::string &format);

/* print network information */
void
print_network (network_ptr net_ptr);

void
load_node (network_ptr net_ptr, node_ptr n_ptr, const boost::property_tree::ptree &pt);

void
load_connection (connection_ptr c_ptr, const boost::property_tree::ptree &pt);

void
reverse_connection (connection_ptr c_ptr, connection_ptr reverse_ptr);

void
load_ns3_application (ns3_application_ptr ns3_app_ptr, const boost::property_tree::ptree &pt);

#endif
