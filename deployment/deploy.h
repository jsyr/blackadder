/*
 * parser.h
 *
 *  Created on: 25 Feb 2015
 *      Author: parisis
 */

#ifndef DEPLOY_H_
#define DEPLOY_H_

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
#include <boost/foreach.hpp>

#include "network.h"

struct network;
struct node;
struct link;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> network_graph;

struct network
{
  int info_id_len;
  int link_id_len;
  bool is_simulation;

  std::map<std::string, struct node> nodes;
  std::vector<struct connection> connections;
  std::vector<struct ns3_application> ns3_applications;

  void
  load_network (const std::string &filename);
};

struct node
{
  std::string click_home;
  std::string conf_home;
  std::string user;
  bool sudo;

  std::string testbed_ip;
  std::string label;
  std::string running_mode;
  std::string operating_system;

  bool is_rv;
  bool is_tm;

  void
  load_node (const boost::property_tree::ptree &pt);
};

struct connection
{
  std::string overlay_mode;

  std::string src_label; 	//read from configuration file
  std::string dst_label; 	//read from configuration file

  std::string src_if; 		//read from configuration file /*e.g. tap0 or eth1*/
  std::string dst_if; 		//read from configuration file /*e.g. tap0 or eth1*/

  std::string src_ip; 		//read from configuration file /*an IP address - i will not resolve mac addresses in this case*/
  std::string dst_ip; 		//read from configuration file /*an IP address - i will not resolve mac addresses in this case*/

  std::string src_mac; 		//will be retrieved using ssh
  std::string dst_mac; 		//will be retrieved using ssh

  void
  load_connection (const boost::property_tree::ptree &pt);
};

/* an ns-3 application */
struct ns3_application
{
  void
  load_ns3_application (const boost::property_tree::ptree &pt);
};

#endif /* DEPLOY_H_ */
