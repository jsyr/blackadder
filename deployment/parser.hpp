/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#ifndef PARSER_HPP
#define	PARSER_HPP

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <libconfig.h++>

#include "network.hpp"

using namespace std;
using namespace libconfig;

/**@brief (Deployment Application) A parser that parses the topology configuration file (that complies with libconfig++) and builds a custom representation of the network (see Domain).
 * 
 * it reads the configuration file, parse the global parameters, add all network nodes and connections.
 */
class parser
{
public:
  /**
   * @brief Constructor: nothing special.
   */
  parser (char *file, network *net);
  /**
   * @brief Destructor: nothing special.
   */
  virtual
  ~parser ();
  /**
   * @brief It parses the configuration file and creates a Config object.
   *
   * @return -1 if the file was not syntactically correct.
   */
  int
  parse_configuration ();
  /**
   * @brief It builds the custom network representation.
   *
   * It reads the configuration file into a Config object, which is then used to read global parameters, node and connection configuration.
   * @return -1 if any exception is caught during the execution.
   */
  int
  build_network ();
//  /**
//   * @brief It builds the custom network representation for an NS3 simulation - different stuff is expected in the topology description.
//   *
//   * It reads the configuration file into a Config object, which is then used to read global parameters, node and connection configuration.
//   * @return -1 if any exception is caught during the execution.
//   */
//  int
//  build_ns3_network ();
  /**
   * @brief reads all mandatory global paramaters of the topology.
   *
   * These parameters are:
   *
   * BLACKADDER_ID_LENGTH: the length in bytes of an identifier fragment.
   *
   * LIPSIN_ID_LENGTH: the length in bytes of the LIPSIN identifiers.
   *
   * CLICK_HOME: the full path of the Click root folder.
   *
   * WRITE_CONF: the full path of the folder to which Click configuration and the Topology Manager .graphml files will be written.
   *
   * USER: the Linux user using which the deployment application will ssh all network nodes.
   *
   * SUDO: whether sudo is required for executing remote commands.
   *
   * OVERLAY_MODE: the overlay mode in which blackadder will run (mac or ip).
   * @return
   */
  int
  get_global_network_params ();
//  /**
//   * @brief reads all mandatory global paramaters of the topology.
//   *
//   * @return
//   */
//  int
//  get_global_ns3_network_params ();
//  /**
//   * @brief It parses a node configuration and maps the configured properties to a NetowrkNode.
//   *
//   * it parses and checks:
//   *
//   * label: the node label. If it is there, it is assigned to the NetworkNode. If not a random one is generated.
//   *
//   * testbed_ip: the IP address of the node in the testbed. It will be used to ssh the node or scp files to it.
//   *
//   * running_mode: whether blackadder is running in user or kernel mode in this node.
//   *
//   * operating_system: operating system in the node. Supported systems: Linux (default), FreeBSD, Darwin.
//   *
//   * name: the node's name (optional). E.g., a brief, human-readable and descriptive name.
//   *
//   * role: the roles of a NetworkNode. A node can be an RV or/and TM. All other nodes are treated the same way. Only one TM and RV must exist in a Domain.
//   *
//   * connections: For each network connection, a NetworkConnection is a added in the NetworkNode, using the addConnection() method.
//   *
//   * @param node A reference to a Setting, which in libconfig++ represents a specific entity in the configuration file (here, being the network node).
//   * @return -1 if something is wrong.
//   */
//  int
//  add_node (const Setting &node);
//  int
//  add_ns3_node (const Setting &node);
//
//  int
//  add_ns3_application (const Setting &application, network_node *nn);
//  /**
//   * @brief It parses and checks a connection configuration and maps the configured properties to a NetworkConnection.
//   *
//   * it parses and checks:
//   *
//   * to: the node label of the destination node.
//   *
//   * Depending on the overlay mode:
//   *
//   * src_if: the source ethernet interface (e.g. eth0, tap0).
//   *
//   * src_ip: the source IP address for the raw IP socket
//   *
//   * dst_if: the destination ethernet interface (e.g. eth0, tap0).
//   *
//   * dst_ip: the destination IP address for the raw IP socket
//   *
//   * src_mac: the source MAC Address interface - if assigned deployment will not ssh to learn this address.
//   *
//   * dst_mac: the destination MAC Address interface - if assigned deployment will not ssh to learn this address.
//   *
//   * @param connection A reference to a Setting, which in libconfig++ represents a specific entity in the configuration file (here, being the network connection).
//   * @param nn a pointer to the NetworkNode for which this connection is added.
//   * @return -1 if something is wrong.
//   */
//  int
//  add_connection (const Setting &connection, network_node *nn);
//
//  int
//  add_ns3_connection (const Setting &connection, network_node *nn);

private:
  char *file;
  Config cfg;
  network *net;
};

#endif	/* PARSER_HPP */
