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

#include "deploy.h"

void
create_graph (network &network, network_graph &net_graph)
{
  vertex src_v, dst_v;

  map<string, vertex>::iterator vertices_map_iter;

  /* iterate over all nodes in net_graph and add respective vertices in the graph*/
  BOOST_FOREACH(node_map_pair_t node_pair, network.nodes) {
    node_ptr src_n_ptr = node_pair.second;
    /* check if this node is already added in the network graph */
    vertices_map_iter = vertices_map.find (src_n_ptr->label);
    if (vertices_map_iter == vertices_map.end ()) {
      /* add the node in the boost graph */
      src_v = add_vertex (src_n_ptr, net_graph);
      vertices_map.insert (pair<string, vertex> (src_n_ptr->label, src_v));
    } else {
      src_v = (*vertices_map_iter).second;
    }
    /* iterate over all connections in net_graph and add respective edges in the graph */
    BOOST_FOREACH(connection_map_pair_t connection_pair, src_n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;
      node_ptr dst_n_ptr = network.nodes[c_ptr->dst_label];
      /* check if destination node is a vertex in the boost graph */
      vertices_map_iter = vertices_map.find (dst_n_ptr->label);
      if (vertices_map_iter == vertices_map.end ()) {
	/* add the node in the boost graph */
	dst_v = add_vertex (dst_n_ptr, net_graph);
	vertices_map.insert (pair<string, boost::graph_traits<network_graph>::vertex_descriptor> (dst_n_ptr->label, dst_v));
      } else {
	dst_v = (*vertices_map_iter).second;
      }
      /* add the connection in the boost graph */
      add_edge (src_v, dst_v, c_ptr, net_graph);
    }
  }
}

void
calculate_forwarding_id (vertex src_v, vertex dst_v, vector<vertex> &predecessor_vector, network_graph &net_graph, bitvector &lipsin)
{
  vertex predeccesor;
  node_ptr n;

  /* source node is the same as destination */
  if (dst_v == src_v) {
    /* XOR lipsin with dst_v == src_v internal_link_id and return */
    lipsin ^= net_graph[dst_v]->internal_link_id;
    return;
  }

  while (true) {
    /* XOR lipsin with dst_v internal_link_id */
    n = net_graph[dst_v];
    lipsin ^= n->internal_link_id;

    /* find the predeccesor node */
    predeccesor = predecessor_vector[dst_v];
    pair<edge, bool> edge_pair = boost::edge (predeccesor, dst_v, net_graph);
    if (edge_pair.second == false) {
      /* this should never happen - an edge must always exist */
      cout << "Fatal: No edge between " << net_graph[predeccesor]->label << " and " << net_graph[dst_v]->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* XOR with edge's link_id */
    lipsin ^= net_graph[edge_pair.first]->link_id;

    /* done */
    if (predeccesor == src_v) {
      break;
    }

    /* move on to the next iteration */
    dst_v = predeccesor;
  }
}

void
calculate_forwarding_ids (network &network, network_graph &net_graph)
{
  /* a predecessor vector used by BFS */
  vector<vertex> predecessor_vector (boost::num_vertices (net_graph));

  /* get RV and TM nodes */
  node_ptr rv_node = network.rv_node;
  node_ptr tm_node = network.tm_node;

  /* get vertex descriptors for these nodes */
  vertex rv_vertex = (*vertices_map.find (rv_node->label)).second;
  vertex tm_vertex = (*vertices_map.find (tm_node->label)).second;

  /* for each node in the network, find the shortest path to rv and tm */
  BOOST_FOREACH(node_map_pair_t node_pair, network.nodes) {
    /* a shared pointer to each node in the network */
    node_ptr src_n_ptr = node_pair.second;

    /* the respective vertex in the boost graph for each node in the network */
    vertex source_vertex = (*vertices_map.find (src_n_ptr->label)).second;

    /* all weights are 1 so, as boost suggests, I am running a BFS with a predecessor map */
    boost::breadth_first_search (net_graph, source_vertex, boost::visitor (boost::make_bfs_visitor (boost::record_predecessors (&predecessor_vector[0], boost::on_tree_edge ()))));

    /* calculate lipsin identifier to the rv node - use the predecessor map above */
    src_n_ptr->lipsin_rv = bitvector (network.link_id_len * 8);
    calculate_forwarding_id (source_vertex, rv_vertex, predecessor_vector, net_graph, src_n_ptr->lipsin_rv);

    /* calculate lipsin identifier to the tm node - use the predecessor map above */
    src_n_ptr->lipsin_tm = bitvector (network.link_id_len * 8);
    calculate_forwarding_id (source_vertex, tm_vertex, predecessor_vector, net_graph, src_n_ptr->lipsin_tm);
  }
}

void
print_graph (network_graph &net_graph)
{
  vertex src_v, dst_v;
  edge e;

  pair<vertex_iter, vertex_iter> vp;
  pair<out_edge_iter, out_edge_iter> ep;

  cout << "------------------BOOST GRAPH--------------------" << endl;
  for (vp = vertices (net_graph); vp.first != vp.second; ++vp.first) {
    src_v = *vp.first;
    cout << "Node " << net_graph[src_v]->label << " connected to nodes:" << endl;
    for (ep = out_edges (src_v, net_graph); ep.first != ep.second; ++ep.first) {
      e = *(ep.first);
      dst_v = target (e, net_graph);
      cout << net_graph[dst_v]->label << "  ";
    }
    cout << endl;
  }
  cout << "-------------------------------------------------" << endl;
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

    if (vm.count ("enable-dump")) enable_dump = true;

    if (vm.count ("no-discover")) no_discover = true;

    if (vm.count ("no-copy")) no_copy = true;

    if (vm.count ("no-start")) no_start = true;

    if (vm.count ("monitor")) monitor = true;

    boost::program_options::notify (vm);

  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << desc << endl;
    return EXIT_FAILURE;
  }

  /* load the network using the provided configuration file */
  network.load (conf, "xml");
  /* assign Link Identifiers and internal link identifiers */
  network.assign_link_ids ();

  /* discover MAC addresses (when needed) for each connection in the network domain */
  if (!network.is_simulation) if (!no_discover)
    network.discover_mac_addresses ();
  else
    network.assign_mac_addresses ();

  /* create boost graph using the network constructed above */
  create_graph (network, network_graph);

  /* calculate forwarding identifiers to the RV and TM */
  calculate_forwarding_ids (network, network_graph);

  /* write all Click/Blackadder configuration files */
  if (!network.is_simulation)
    ; //    dm.write_click_conf (monitor, enable_dump);
  else
    ; //    dm.write_ns3_click_conf ();

  /* copy Click configuration files to remote nodes */
  if (!network.is_simulation) {
    if (!no_copy) ; // dm.scp_click_conf ();

    /* start Click using the copied configuration file */
    if (!no_start) ; // dm.start_click ();
  }

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

  if (!network.is_simulation) {
    /* copy the .graphml file to the Topology Manager node */
    if (!no_copy) ; // dm.scp_tm_conf ("topology.graphml");

    /* start the Topology Manager */
    if (!no_start) ; // dm.startTM();
  }

  if (network.is_simulation) ; // dm.create_ns3_code ();

  /* print boost graph to debug */
  print_graph (network_graph);

  /* print network to debug */
  network.print ();
}
