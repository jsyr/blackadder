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

  /* a network struct to populate using the configuration file and boost property tree */
  struct network network;

  /* name of configuration file */
  string conf;

  bool no_discover = false;
  bool no_copy = false;
  bool no_start = false;

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("blackadder deployment tool options");

  desc.add_options () ("help,h", "Print help message");
  desc.add_options () ("conf,c", boost::program_options::value<string> (&conf)->required (), "Configuration file (mandatory)");
  desc.add_options () ("no-discover", "Don't auto-discover MAC addresses");
  desc.add_options () ("no-copy", "Don't copy Click and TM conf files to remote nodes");
  desc.add_options () ("no-start", "Don't start Click and TM at remote nodes");

  /* parse command line arguments */
  try {

    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) cout << desc << endl;

    if (vm.count ("no-discover")) no_discover = true;

    if (vm.count ("no-copy")) no_copy = true;

    if (vm.count ("no-start")) no_start = true;

    boost::program_options::notify (vm);

  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << endl << desc << endl;
    return EXIT_FAILURE;
  }

  /* load the network using the provided configuration file */
  network.load (conf, "xml");

  /* assign Link Identifiers and internal link identifiers */
  network.assign_link_ids ();

  /* discover MAC addresses (when/if needed) for each connection in the network domain */
  if (!network.is_simulation) if (!no_discover) network.discover_mac_addresses ();
  // else
  // network.assign_mac_addresses ();

  /* create boost graph using the network constructed above */
  network.create_graph ();

  /* calculate forwarding identifiers to the RV and TM */
  network.calculate_forwarding_ids ();

  /* write all Click/Blackadder configuration files */
  network.write_click_conf ();

  /* store the graph in graphml format for the topology manager */
  network.write_tm_conf ();

  if (!network.is_simulation) {
    /* copy Click configuration files to remote nodes */
    if (!no_copy) network.scp_click_conf ();

    /* copy the .graphml file to the Topology Manager node */
    if (!no_copy) network.scp_tm_conf ("topology.graphml");

    /* start Click using the copied configuration file */
    if (!no_start) network.start_click ();

    /* start the Topology Manager to the remote node */
    if (!no_start) network.start_tm ();
  } else {
    // dm.create_ns3_code ();
    /* compile and run the ns-3 topology separately */
  }

  /* print boost graph to debug */
  network.print_graph ();

  /* print network to debug */
  network.print ();
}
