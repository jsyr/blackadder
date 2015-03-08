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

#include <boost/foreach.hpp>

#include <map>

#include "bitvector.h"

using namespace std;

struct network;
struct node;
struct connection;
struct ns3_application;

typedef boost::shared_ptr<node> node_ptr;
typedef boost::shared_ptr<connection> connection_ptr;
typedef boost::shared_ptr<ns3_application> ns3_application_ptr;

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

  map<string, node_ptr> nodes;

  node_ptr rv_node;
  node_ptr tm_node;

  /* used internally */

  void
  load (const string &filename, const string &format);
  /* assign Link Identifiers and internal link identifiers */
  void
  assign_link_ids ();
  void
  calculate_lid (map<string, bitvector>& link_identifiers, int index);

  void
  discover_mac_addresses ();
  void
  assign_mac_addresses ();
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
  multimap<string, connection_ptr> connections;

  /* if is_simulation == true */
  vector<ns3_application_ptr> ns3_applications;
  int device_offset;

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

  string src_label;
  string dst_label;

  string src_if; /* e.g. tap0 or eth1 */
  string dst_if; /* e.g. tap0 or eth1 */

  string src_ip; /* an IP address - i will not resolve mac addresses in this case */
  string dst_ip; /* an IP address - i will not resolve mac addresses in this case */

  string src_mac;
  string dst_mac;

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
  string node_label;

  void
  load (const boost::property_tree::ptree &pt);
};

#endif
