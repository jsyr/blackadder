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

int
main (int argc, char **argv)
{
  int ret;

  /* a boost bidirectional, directed graph to use throughout blackadder deployment */
  graph network_graph;
  /* input file stream to read initial graphml configuration */
  std::ifstream in;
  /* dynamic properties to set when parsing .graphml configuration file */
  boost::dynamic_properties properties;

  /* name of configuration file */
  string conf;

  bool simulation = false;
  bool enable_dump = false;
  bool no_discover = false;
  bool no_copy = false;
  bool no_start = false;

  bool monitor = false;

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("blackadder deployment tool options");
  desc.add_options () ("help,h", "Print help message");

  desc.add_options () ("conf,c", boost::program_options::value<string> (&conf)->required (), "Configuration file (mandatory)");

  desc.add_options () ("simulation", "Output ns-3 simulation code");
  desc.add_options () ("enable_dump", "Enable RV info dump support");
  desc.add_options () ("no-discover", "Don't auto-discover MAC addresses");
  desc.add_options () ("no-copy", "Don't copy Click configuration files to remote nodes");
  desc.add_options () ("no-start", "Don't start Click at remote nodes");

  desc.add_options () ("monitor,m", "Enable Click monitor tool in blackadder nodes");

  /* parse command line arguments */
  try {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) cout << desc << endl;

    if (vm.count ("simulation")) simulation = true;

    if (vm.count ("enable_dump")) enable_dump = true;

    if (vm.count ("no_discover")) no_discover = true;

    if (vm.count ("no_copy")) no_copy = true;

    if (vm.count ("no_start")) no_start = true;

    if (vm.count ("monitor")) monitor = true;

    boost::program_options::notify (vm);
  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << endl;
    cerr << desc << endl;
    return -1;
  }

  /* open the input stream for readind */
  in.open (conf.c_str (), std::ifstream::in);

  properties.property ("test_graph", boost::get (&graph_properties::test_graph, network_graph));
  properties.property ("test_vertex", boost::get (&vertex_properties::test_vertex, network_graph));
  properties.property ("test_edge", boost::get (&edge_properties::test_edge, network_graph));

  /* input .graphml file must contain a single graph definition */
  try {
    read_graphml (in, network_graph, properties);
  } catch (const boost::bad_parallel_edge& ex) {
    cout << "Exception:" << ex.what () << endl;
    return EXIT_FAILURE;
  } catch (const boost::directed_graph_error& ex) {
    cout << "Exception:" << ex.what () << endl;
    return EXIT_FAILURE;
  } catch (const boost::undirected_graph_error& ex) {
    cout << "Exception:" << ex.what () << endl;
    return EXIT_FAILURE;
  } catch (const boost::parse_error& ex) {
    cout << "Exception:" << ex.what () << endl;
    return EXIT_FAILURE;
  }

  cout << network_graph[boost::graph_bundle].test_graph << endl;

  graph::vertex_descriptor v = *vertices (network_graph).first;
  cout << network_graph[v].test_vertex << endl;

//  /* create an empty network representation */
//  network net = network ();
//
//  /* create a parser object */
//  parser parser (((char *) conf.c_str ()), &net);
//
//  if (!simulation) ret = parser.build_network ();
////  else
////    ret = parser.build_ns3_network ();
//
//  /* check for errors */
//  if (ret < 0) {
//    cout << "Something went wrong: " << ret << endl;
//    return EXIT_FAILURE;
//  }

//
//
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
