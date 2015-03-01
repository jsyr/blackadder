/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
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

#include "deploy.h"

using namespace std;

void
network::load_network (const string &filename)
{
  // Create an empty property tree object
  using boost::property_tree::ptree;
  ptree pt;

  // Load the XML file into the property tree. If reading fails
  // (cannot open file, parse error), an exception is thrown.
  read_xml (filename, pt);

  /* parse network parameters */
  try {
    info_id_len = pt.get<int> ("network.info_id_len");
    link_id_len = pt.get<int> ("network.link_id_len");
    is_simulation = pt.get<int> ("network.is_simulation", false);
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  }

  cout << "info_id_len:   " << info_id_len << endl;
  cout << "link_id_len:   " << link_id_len << endl;
  cout << "is_simulation: " << is_simulation << endl;

  try {
    BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.nodes")) {
      node n;
      n.load_node (v.second);
      nodes.insert (pair<string, struct node> (n.label, n));
    }
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "No nodes are defined - aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  try {
    BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.connections")) {
      connection c;
      c.load_connection (v.second);
      connections.push_back (c);
    }
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "No connections are defined " << endl;
  }

  if (is_simulation) {
    try {
      BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.applications")) {
	ns3_application a;
	a.load_ns3_application (v.second);
	ns3_applications.push_back (a);
      }
    } catch (boost::property_tree::ptree_bad_data& err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    } catch (boost::property_tree::ptree_bad_path& err) {
      cerr << "No ns-3 applications are defined " << endl;
      exit (EXIT_FAILURE);
    }
  }
}

void
node::load_node (const boost::property_tree::ptree &pt)
{
  try {
    /* mandatory */
    label = pt.get<string> ("label");
    testbed_ip = pt.get<string> ("testbed_ip");
    user = pt.get<string> ("user");

    /* optional - with default values */
    sudo = pt.get<bool> ("sudo", false);
    click_home = pt.get<string> ("click_home", "/home/" + user + "/click");
    conf_home = pt.get<string> ("conf_home", "/home/" + user + "/conf");
    running_mode = pt.get<string> ("running_mode", "user");
    operating_system = pt.get<string> ("operating_system", "linux");
    is_rv = pt.get<bool> ("is_rv", false);
    is_tm = pt.get<bool> ("is_tm", false);

  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory node parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }

  cout << "label:      " << label << endl;
  cout << "testbed_ip: " << testbed_ip << endl;
  cout << "user:       " << user << endl;

  cout << "click_home: " << click_home << endl;
  cout << "conf_home:  " << conf_home << endl;
  cout << "sudo:       " << sudo << endl;
  cout << "mode:       " << running_mode << endl;
  cout << "os:         " << operating_system << endl;
}

void
connection::load_connection (const boost::property_tree::ptree &pt)
{
  try {
    /* mandatory */
    src_label = pt.get<string> ("src_label");
    dst_label = pt.get<string> ("dst_label");

    /* optional - with default values */
    overlay_mode = pt.get<string> ("overlay_mode", "Ethernet");

    src_if = pt.get<string> ("src_if", "unspecified");
    dst_if = pt.get<string> ("dst_if", "unspecified");

    src_mac = pt.get<string> ("src_mac", "unspecified");
    dst_mac = pt.get<string> ("dst_mac", "unspecified");

    src_ip = pt.get<string> ("src_ip", "unspecified");
    dst_ip = pt.get<string> ("dst_ip", "unspecified");

  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory connection parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }

  cout << "src_label:    " << src_label << endl;
  cout << "dst_label:    " << src_label << endl;

  cout << "overlay_mode: " << overlay_mode << endl;
  cout << "src_if:       " << src_if << endl;
  cout << "dst_if:       " << dst_if << endl;
  cout << "src_mac:      " << src_mac << endl;
  cout << "dst_mac:      " << dst_mac << endl;
  cout << "src_ip:       " << src_ip << endl;
  cout << "dst_ip:       " << dst_ip << endl;
}

void
ns3_application::load_ns3_application (const boost::property_tree::ptree &pt)
{

}

int
main (int argc, char **argv)
{
  /* a network struct to populate using the configuration file and boost property tree */
  struct network network;

  /* a boost bidirectional, directed graph to use throughout blackadder deployment */
  network_graph network_graph;

  int ret;

  /* name of configuration file */
  string conf;

  bool enable_dump = false;
  bool no_discover = false;
  bool no_copy = false;
  bool no_start = false;
  bool monitor = false;

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("blackadder deployment tool options");
  desc.add_options () ("help,h", "Print help message");

  desc.add_options () ("conf,c", boost::program_options::value<string> (&conf)->required (), "Configuration file (mandatory)");

  desc.add_options () ("enable_dump", "Enable RV info dump support");
  desc.add_options () ("no-discover", "Don't auto-discover MAC addresses");
  desc.add_options () ("no-copy", "Don't copy Click configuration files to remote nodes");
  desc.add_options () ("no-start", "Don't start Click at remote nodes");

  desc.add_options () ("monitor,m", "Enable Click monitor tool in blackadder nodes");

  /* parse command line arguments */
  try {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) cout << desc << endl;

    if (vm.count ("enable_dump")) enable_dump = true;

    if (vm.count ("no_discover")) no_discover = true;

    if (vm.count ("no_copy")) no_copy = true;

    if (vm.count ("no_start")) no_start = true;

    if (vm.count ("monitor")) monitor = true;

    boost::program_options::notify (vm);
  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << desc << endl;
    return EXIT_FAILURE;
  }

  network.load_network (conf);

//  /* create a graph representation of the network domain */
//  GraphRepresentation graph = GraphRepresentation (&dm);
//
//  /* assign Link Identifiers and internal link identifiers */
//  dm.assign_lids ();
//
//  /* transform the network domain representation to an iGraph representation */
//  graph.buildIGraphTopology ();
//
//  /* calculate the default forwarding identifiers from each node to the domain's Rendezvous Node */
//  graph.calculateRVFIDs ();
//
//  /* calculate the default forwarding identifiers from each node to the domain's Topology Manager */
//  graph.calculateTMFIDs ();
//
//  /* discover MAC addresses (when needed) for each connection in the network domain */
//  if (!simulation)
//    dm.discover_mac_addresses (no_discover);
//  else
//    dm.assign_device_names_mac_addresses ();
//
//  /* write all Click/Blackadder configuration files */
//  if (!simulation)
//    dm.write_click_conf (monitor, enable_dump);
//  else
//    dm.write_ns3_click_conf ();
//
//  /* copy Click configuration files to remote nodes */
//  if (!simulation) {
//    if (!no_copy) dm.scp_click_conf ();
//
//    /* start Click using the copied configuration file */
//    if (!no_start) dm.start_click ();
//
//  }
//  /* set some graph attributes for the topology manager */
//  igraph_cattribute_GAN_set (&graph.igraph, "FID_LEN", dm.fid_len);
//  igraph_cattribute_GAS_set (&graph.igraph, "TM", dm.TM_node->label.c_str ());
//  igraph_cattribute_GAS_set (&graph.igraph, "RV", dm.RV_node->label.c_str ());
//
//  cout << "TM is " << dm.TM_node->label << endl;
//  cout << "RV is " << dm.RV_node->label << endl;
//
//  igraph_cattribute_GAS_set (&graph.igraph, "TM_MODE", dm.TM_node->running_mode.c_str ());
//  FILE * outstream_graphml = fopen (string (dm.write_conf + "topology.graphml").c_str (), "w");
//
//  /* the following happens if blackadder had been previously deployed from another machine */
//  /* so the file exists with root permissions */
//  if (outstream_graphml == NULL) {
//    cout << "Could not open file " << string (dm.write_conf + "topology.graphml") << "\n";
//    cout << "Try 'sudo rm " << string (dm.write_conf + "topology.graphml") << "' and re-deploy...\n";
//    return EXIT_FAILURE;
//  }
//
//  igraph_write_graph_graphml (&graph.igraph, outstream_graphml);
//  fclose (outstream_graphml);
//
//  if (!simulation) {
//    /* copy the .graphml file to the Topology Manager node */
//    if (!no_copy) dm.scp_tm_conf ("topology.graphml");
//
//    /* start the Topology Manager */
//    //dm.startTM();
//  }
//
//  if (simulation) dm.create_ns3_code ();

  cout << "Done!" << endl;
}
