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

using namespace std;

int
main (int argc, char **argv)
{
  int ret;

  /* a shared_ptr to a network struct to populate using the configuration file and boost property tree */
  network_ptr net_ptr (new network ());

  /* a boost bidirectional, directed graph to use throughout blackadder deployment */
  network_graph_ptr net_graph_ptr (new network_graph (net_ptr));

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("blackadder deployment tool options");

  desc.add_options () ("help,h", "Print help message");
  desc.add_options () ("conf,c", boost::program_options::value<string> (&conf)->required (), "Configuration file (required)");
  desc.add_options () ("out_folder,o", boost::program_options::value<string> (&output_folder), "Folder to store Click files and Topology Graph (Default: /tmp)");
  desc.add_options () ("format,f", boost::program_options::value<string> (&format), "Configuration file format (xml,json,ini,info) (Default: xml)");
  desc.add_options () ("no-discover", "Don't auto-discover MAC addresses (Default: false)");
  desc.add_options () ("no-copy", "Don't copy Click and TM conf files to remote nodes (Default: false)");
  desc.add_options () ("no-start", "Don't start Click and TM at remote nodes (Default: false)");
  desc.add_options () ("verbose,v", "Print Network and Graph structures (Default: false)");

  /* parse command line arguments */
  try {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) {
      cout << desc << endl;
      return EXIT_SUCCESS;
    }

    if (vm.count ("no-discover")) no_discover = true;

    if (vm.count ("no-copy")) no_copy = true;

    if (vm.count ("no-start")) no_start = true;

    if (vm.count ("verbose")) verbose = true;

    boost::program_options::notify (vm);
  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << endl << desc << endl;
    return EXIT_FAILURE;
  }

  /* load the network using the provided configuration file (boost property tree) */
  load_network (net_ptr, conf, format);

  /* create boost graph using the network constructed above */
  create_graph (net_graph_ptr, net_ptr);

  /* assign Link Identifiers and internal link identifiers */
  assign_link_ids (net_graph_ptr);

  /* calculate forwarding identifiers to RV and TM */
  calculate_forwarding_ids (net_graph_ptr);

  /* discover MAC addresses (when/if needed) for each connection in the network domain */
  if (!net_ptr->is_simulation) if (!no_discover) discover_mac_addresses (net_graph_ptr);
  // TODO: ns-3 support
  // else
  // network.assign_mac_addresses ();

  /* create all Click/Blackadder configuration files and store them in the tmp_conf_folder (set in the configuration file) */
  // TODO: ns-3 support
  write_click_conf (net_graph_ptr, output_folder);

  /* create the graph in graphviz format for the topology manager and store it in the tmp_conf_folder (set in the configuration file) */
  write_tm_conf (net_graph_ptr, output_folder);

  if (!net_ptr->is_simulation) {

    if (!no_copy) {
      /* copy Click configuration files to remote nodes */
      scp_click_conf (net_graph_ptr, output_folder);

      /* copy the .graphml file to the Topology Manager node */
      scp_tm_conf (net_graph_ptr, output_folder);
    }

    if (!no_start) {
      /* start Click using the copied configuration file */
      start_click (net_graph_ptr);

      /* start the Topology Manager to the remote node */
      start_tm (net_graph_ptr);
    }

  } else {
    // TODO: ns-3 support
    // dm.create_ns3_code ();
    /* compile and run the ns-3 topology separately */
  }

  if (verbose) {
    /* print network structure */
    cout << endl;
    print_network (net_ptr);
  }
}
