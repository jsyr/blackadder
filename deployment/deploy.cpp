/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 * PlanetLab support By Dimitris Syrivelis
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#include <boost/program_options.hpp>

#include "network.hpp"
#include "graph_representation.hpp"
#include "parser.hpp"

using namespace std;

int
main (int argc, char **argv)
{
  int ret;

  /* name of configuration file */
  string conf;

  /*  */
  string tgz;

  bool simulation = false;
  bool enable_dump = false;
  bool no_discover = false;
  bool no_copy = false;
  bool no_start = false;

  bool autogenerate = false;
  bool transfer_binaries = false;
  bool monitor = false;

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("blackadder deployment tool options");
  desc.add_options () ("help,h", "Print help message");

  desc.add_options () ("conf,c", boost::program_options::value<string> (&conf)->required (), "Configuration file (mandatory)");
  desc.add_options () ("tgz,t", boost::program_options::value<string> (&tgz), "compressed file to be transferred and extracted at home folders on remote nodes");

  desc.add_options () ("simulation", "Output ns-3 simulation code");
  desc.add_options () ("enable_dump", "Enable RV info dump support");
  desc.add_options () ("no_discover", "Don't auto-discover MAC addresses");
  desc.add_options () ("no_copy", "Don't copy Click configuration files to remote nodes");
  desc.add_options () ("no_start", "Don't start Click at remote nodes");

  desc.add_options () ("auto,a", "Enable graph auto-generation");
  desc.add_options () ("monitor,m", "Enable Click monitor tool in blackadder nodes");

  /* parse command line arguments */
  try
  {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help"))
      cout << desc << endl;

    if (vm.count ("simulation"))
      simulation = true;

    if (vm.count ("enable_dump"))
      enable_dump = true;

    if (vm.count ("no_discover"))
      no_discover = true;

    if (vm.count ("no_copy"))
      no_copy = true;

    if (vm.count ("auto"))
      monitor = true;

    if (vm.count ("monitor"))
      monitor = true;

    boost::program_options::notify (vm);
  }
  catch (boost::program_options::error& e)
  {
    cerr << "ERROR: " << e.what () << endl;
    cerr << desc << endl;
    return -1;
  }

  string autoconffile = "";
  if (autogenerate)
  {
    /* create an empty network representation */
    Domain domain = Domain ();

    /* create a parser object */
    Parser parser (((char *) conf.c_str ()), &domain);

    /* build PlanetLab Domain out of the planetlab available node file input */
    ret = parser.buildPlanetLabDomain ();
    ret = parser.getGlobalDomainParameters ();

    /* check for errors */
    if (ret < 0)
    {
      cout << "Something went wrong: " << ret << endl;
      return EXIT_FAILURE;
    }

    /* create a graph representation of the network domain */
    /* if autogenerated is true, an igraph instance will be  created using the Barabasi-Albert model */
    GraphRepresentation graph = GraphRepresentation (&domain, autogenerate);

    /** the igraph barabasi instance previously created will be traversed, to build
     the deployment tool internal representation so that click configuration file generation and deployment execution
     can be carried out by the same code with the non-autogenerated graph input.
     */
    graph.buildInputMap ();
    graph.chooseBestTMRVNode ();

    /* Write standard configuration file format */
    autoconffile = domain.writeConfigFile ("autogenerated.cfg");

    /* calculate leaf vertices */
    graph.outputLeafVertices ("edgevertices.cfg");
  }

  /* create an empty network representation */
  Domain dm = Domain ();
  /* create a parser object. the configuration file and the network domain are the parameters */

  string parserinputfile = "";
  if (autogenerate)
  {
    parserinputfile = autoconffile;
  }
  else
  {
    parserinputfile = conf;
  }

  Parser parser (((char *) parserinputfile.c_str ()), &dm);

  if (!simulation)
    ret = parser.buildNetworkDomain ();
  else
    ret = parser.buildNS3NetworkDomain ();

  /* check for errors */
  if (ret < 0)
  {
    cout << "Something went wrong: " << ret << endl;
    return EXIT_FAILURE;
  }

  /* create a graph representation of the network domain */
  /* if autogenerated is true, an igraph instance will be now created using the Barabasi-Albert model */
  GraphRepresentation graph = GraphRepresentation (&dm, autogenerate);

  /* assign Link Identifiers and internal link identifiers */
  dm.assignLIDs ();

  /* transform the network domain representation to an iGraph representation */
  graph.buildIGraphTopology ();

  /* calculate the default forwarding identifiers from each node to the domain's Rendezvous Node */
  graph.calculateRVFIDs ();

  /* calculate the default forwarding identifiers from each node to the domain's Topology Manager */
  graph.calculateTMFIDs ();

  /* discover MAC addresses (when needed) for each connection in the network domain */
  if (!simulation)
    dm.discoverMacAddresses (no_discover);
  else
    dm.assignDeviceNamesAndMacAddresses ();

  /* write all Click/Blackadder configuration files */
  if (!simulation)
    dm.writeClickFiles (monitor, enable_dump);
  else
    dm.writeNS3ClickFiles ();

  /* tranfer a .tgz file from path to all nodes and also issue a tar zxvf command */
  if (transfer_binaries and !no_copy)
    dm.scpClickBinary (tgz);

  /* copy Click configuration files to remote nodes */
  if (!simulation)
  {
    if (!no_copy)
      dm.scpClickFiles ();

    /* start Click using the copied configuration file */
    if (!no_start)
      dm.startClick ();

  }
  /* set some graph attributes for the topology manager */
  igraph_cattribute_GAN_set (&graph.igraph, "FID_LEN", dm.fid_len);
  igraph_cattribute_GAS_set (&graph.igraph, "TM", dm.TM_node->label.c_str ());
  igraph_cattribute_GAS_set (&graph.igraph, "RV", dm.RV_node->label.c_str ());

  cout << "TM is " << dm.TM_node->label << endl;
  cout << "RV is " << dm.RV_node->label << endl;

  igraph_cattribute_GAS_set (&graph.igraph, "TM_MODE", dm.TM_node->running_mode.c_str ());
  FILE * outstream_graphml = fopen (string (dm.write_conf + "topology.graphml").c_str (), "w");

  /* the following happens if blackadder had been previously deployed from another machine */
  /* so the file exists with root permissions */
  if (outstream_graphml == NULL)
  {
    cout << "Could not open file " << string (dm.write_conf + "topology.graphml") << "\n";
    cout << "Try 'sudo rm " << string (dm.write_conf + "topology.graphml") << "' and re-deploy...\n";
    return EXIT_FAILURE;
  }

  igraph_write_graph_graphml (&graph.igraph, outstream_graphml);
  fclose (outstream_graphml);

  if (!simulation)
  {
    /* copy the .graphml file to the Topology Manager node */
    if (!no_copy)
      dm.scpTMConfiguration ("topology.graphml");

    /* start the Topology Manager */
    //dm.startTM();
  }

  if (simulation)
    dm.createNS3SimulationCode ();

  cout << "Done!" << endl;
}
