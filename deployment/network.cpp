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

#include "network.h"

using namespace std;

void
parse_configuration (boost::property_tree::ptree &pt, const string &filename, const string &format)
{
  boost::filesystem::path conf_path (filename);
  boost::filesystem::ifstream conf_path_stream;

  if (!boost::filesystem::exists (conf_path)) {
    cerr << "Path " << conf_path << " does not exist. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  if (!boost::filesystem::is_regular (conf_path)) {
    cerr << "Path " << conf_path << " is not a regular file. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  conf_path_stream.open (conf_path);

  if (format.compare ("xml") == 0) {
    try {
      // Load the .xml file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_xml (conf_path_stream, pt);
    } catch (boost::property_tree::xml_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("json") == 0) {
    try {
      // Load the .json file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_json (conf_path_stream, pt);
    } catch (boost::property_tree::json_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("ini") == 0) {
    try {
      // Load the .ini file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_ini (conf_path_stream, pt);
    } catch (boost::property_tree::ini_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("info") == 0) {
    try {
      // Load the .info file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_info (conf_path_stream, pt);
    } catch (boost::property_tree::info_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else {
    cerr << "unrecognised configuration format (accepted values are xml, json, ini, info)" << endl;
    exit (EXIT_FAILURE);
  }
}

void
load_network (network_ptr net_ptr, const string &filename, const string &format)
{
  // Create an empty property tree object
  using boost::property_tree::ptree;
  ptree pt;

  /* parse configuration file */
  parse_configuration (pt, filename, format);

  /* get network parameters */
  try {
    net_ptr->info_id_len = pt.get<int> ("network.info_id_len");
    net_ptr->link_id_len = pt.get<int> ("network.link_id_len");
    net_ptr->is_simulation = pt.get<bool> ("network.is_simulation", false);

    net_ptr->user = pt.get<string> ("network.user", "unspecified");
    net_ptr->sudo = pt.get<bool> ("network.sudo", false);
    net_ptr->click_home = pt.get<string> ("network.click_home", "unspecified");
    net_ptr->conf_home = pt.get<string> ("network.conf_home", "unspecified");
    net_ptr->running_mode = pt.get<string> ("network.running_mode", "unspecified");
    net_ptr->operating_system = pt.get<string> ("network.operating_system", "unspecified");
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  }

  try {
    BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.nodes")) {
      node_ptr n_ptr (new node ());
      load_node (net_ptr, n_ptr, v.second);
      pair<map<string, node_ptr>::iterator, bool> ret = net_ptr->nodes.insert (pair<string, node_ptr> (n_ptr->label, n_ptr));
      if (ret.second == false) {
	cerr << "Node " << n_ptr->label << " is a duplicate. Aborting..." << endl;
	exit (EXIT_FAILURE);
      } else {
	if (n_ptr->is_rv == true) {
	  if (net_ptr->rv_node != NULL) {
	    cerr << "Multiple RV nodes are defined. Aborting..." << endl;
	    exit (EXIT_FAILURE);
	  } else {
	    net_ptr->rv_node = n_ptr;
	  }
	}
	if (n_ptr->is_tm == true) {
	  if (net_ptr->tm_node != NULL) {
	    cerr << "Multiple TM nodes are defined. Aborting..." << endl;
	    exit (EXIT_FAILURE);
	  } else {
	    net_ptr->tm_node = n_ptr;
	  }
	}
      }
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
      connection_ptr c_ptr (new connection ());
      map<string, node_ptr>::iterator src_iter;
      load_connection (c_ptr, v.second);

      /* insert connection in src node connection map */
      src_iter = net_ptr->nodes.find (c_ptr->src_label);
      if (src_iter != net_ptr->nodes.end ()) {
	if (net_ptr->nodes.find (c_ptr->dst_label) != net_ptr->nodes.end ()) {
	  src_iter->second->connections.insert (pair<string, connection_ptr> (c_ptr->dst_label, c_ptr));
	} else {
	  cerr << "Node " << c_ptr->dst_label << " does not exist. Aborting..." << endl;
	  exit (EXIT_FAILURE);
	}
      } else {
	cerr << "Node " << c_ptr->src_label << " does not exist. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }

      if (c_ptr->is_bidirectional) {
	/* internally, connections are treated as unidirectional */
	/* insert reversed connection in dst node connection map */
	connection_ptr reverse_ptr (new connection ());
	reverse_connection (c_ptr, reverse_ptr);
	src_iter = net_ptr->nodes.find (reverse_ptr->src_label);
	/* both source and destination nodes exist - checked above */
	src_iter->second->connections.insert (pair<string, connection_ptr> (reverse_ptr->dst_label, reverse_ptr));
      }
    }
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    /* this is allowed to support one node deployments for debugging */
    cerr << "No connections are defined " << endl;
  }

  if (net_ptr->is_simulation) {
    try {
      BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.applications")) {
	ns3_application_ptr a_ptr;
	load_ns3_application (a_ptr, v.second);
	net_ptr->nodes.at (a_ptr->node_label)->ns3_applications.push_back (a_ptr);
      }
    } catch (boost::property_tree::ptree_bad_data& err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    } catch (boost::property_tree::ptree_bad_path& err) {
      cerr << "No ns-3 applications are defined " << endl;
    }
  }

  if (net_ptr->tm_node == NULL) {
    cerr << "No TM node is defined. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }
  if (net_ptr->rv_node == NULL) {
    cerr << "No RV node is defined. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }
}

void
load_node (network_ptr net_ptr, node_ptr n_ptr, const boost::property_tree::ptree &pt)
{
  try {
    /* mandatory */
    n_ptr->label = pt.get<string> ("label");
    if (n_ptr->label.length () != net_ptr->info_id_len) {
      cerr << "Label " << n_ptr->label << " is not " << net_ptr->info_id_len << " bytes long. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    n_ptr->testbed_ip = pt.get<string> ("testbed_ip");

    /* optional - with default values */
    n_ptr->is_rv = pt.get<bool> ("is_rv", false);
    n_ptr->is_tm = pt.get<bool> ("is_tm", false);

    /* user can be set globally for the whole network */
    if (net_ptr->user.compare ("unspecified") == 0) {
      /* user was not specified globally - MUST be set on a per-node basis */
      n_ptr->user = pt.get<string> ("user");
    } else {
      /* if not overriden here, use the global value */
      n_ptr->user = pt.get<string> ("user", net_ptr->user);
    }

    /* sudo can be set globally for the whole network */
    n_ptr->sudo = pt.get<bool> ("user", false);
    try {
      /* if not explicitly set for this node */
      n_ptr->sudo = pt.get<bool> ("sudo");
    } catch (boost::property_tree::ptree_bad_path& err) {
      /* use the value (or the default one) that was set for the whole network */
      n_ptr->sudo = net_ptr->sudo;
    }

    /* click_home can be set globally for the whole network */
    if (net_ptr->click_home.compare ("unspecified") == 0) {
      /* click_home was not specified globally - use the provided or the default value */
      n_ptr->click_home = pt.get<string> ("click_home", "/home/" + n_ptr->user + "/click/");
    } else {
      /* if not overriden here, use the global value */
      n_ptr->click_home = pt.get<string> ("click_home", net_ptr->click_home);
    }

    /* conf_home can be set globally for the whole network */
    if (net_ptr->conf_home.compare ("unspecified") == 0) {
      /* conf_home was not specified globally - use the provided or the default value */
      n_ptr->conf_home = pt.get<string> ("conf_home", "/home/" + n_ptr->user + "/conf");
    } else {
      /* if not overriden here, use the global value */
      n_ptr->conf_home = pt.get<string> ("conf_home", net_ptr->conf_home);
    }

    /* running_mode can be set globally for the whole network */
    if (net_ptr->running_mode.compare ("unspecified") == 0) {
      /* running_mode was not specified globally - use the provided or the default value */
      n_ptr->running_mode = pt.get<string> ("running_mode", "user");
    } else {
      /* if not overriden here, use the global value */
      n_ptr->running_mode = pt.get<string> ("running_mode", net_ptr->running_mode);
    }
    if (!(n_ptr->running_mode.compare ("user") == 0) && !(n_ptr->running_mode.compare ("kernel") == 0)) {
      cerr << "running_mode must be either user or kernel. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }

    /* operating_system can be set globally for the whole network */
    if (net_ptr->operating_system.compare ("unspecified") == 0) {
      /* operating_system was not specified globally - use the provided or the default value */
      n_ptr->operating_system = pt.get<string> ("operating_system", "Linux");
    } else {
      /* if not overriden here, use the global value */
      n_ptr->operating_system = pt.get<string> ("operating_system", net_ptr->operating_system);
    }
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory node parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }
}

void
load_connection (connection_ptr c_ptr, const boost::property_tree::ptree &pt)
{
  try {
    /* mandatory */
    c_ptr->src_label = pt.get<string> ("src_label");
    c_ptr->dst_label = pt.get<string> ("dst_label");

    if (c_ptr->src_label.compare (c_ptr->dst_label) == 0) {
      cerr << "src_label and dst_label must not be the same. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }

    /* optional - with default value */
    c_ptr->overlay_mode = pt.get<string> ("overlay_mode", "Ethernet");
    if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
      /* mandatory */
      c_ptr->src_if = pt.get<string> ("src_if");
      c_ptr->dst_if = pt.get<string> ("dst_if");
      /* optional */
      c_ptr->src_mac = pt.get<string> ("src_mac", "unspecified");
      c_ptr->dst_mac = pt.get<string> ("dst_mac", "unspecified");
    } else if (c_ptr->overlay_mode.compare ("IP") == 0) {
      /* mandatory */
      c_ptr->src_ip = pt.get<string> ("src_ip");
      c_ptr->dst_ip = pt.get<string> ("dst_ip");
    } else {
      cerr << "Unrecognised overlay_mode. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* by default connections are bidirectional */
    c_ptr->is_bidirectional = pt.get<bool> ("is_bidirectional", "true");
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory connection parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }
}

void
reverse_connection (connection_ptr c_ptr, connection_ptr reverse_ptr)
{
  reverse_ptr->is_bidirectional = c_ptr->is_bidirectional;
  reverse_ptr->overlay_mode = c_ptr->overlay_mode;

  reverse_ptr->src_label = c_ptr->dst_label;
  reverse_ptr->dst_label = c_ptr->src_label;

  reverse_ptr->src_if = c_ptr->dst_if;
  reverse_ptr->dst_if = c_ptr->src_if;

  reverse_ptr->src_mac = c_ptr->dst_mac;
  reverse_ptr->dst_mac = c_ptr->src_mac;

  reverse_ptr->src_ip = c_ptr->dst_ip;
  reverse_ptr->dst_ip = c_ptr->src_ip;
}

// TODO: ns-3 support
void
load_ns3_application (ns3_application_ptr ns3_app_ptr, const boost::property_tree::ptree &pt)
{

}

void
print_network (network_ptr net_ptr)
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  cout << "--------------------NETWORK----------------------" << endl;
  cout << "info_id_len:      " << net_ptr->info_id_len << endl;
  cout << "link_id_len:      " << net_ptr->link_id_len << endl;
  cout << "is_simulation:    " << net_ptr->is_simulation << endl;
  cout << "Topology Manager: " << net_ptr->tm_node->label << endl;
  cout << "Rendezvous Node:  " << net_ptr->rv_node->label << endl;

  BOOST_FOREACH(node_map_pair_t node_pair, net_ptr->nodes) {

    node_ptr n_ptr = node_pair.second;
    cout << "------------------NODE " << n_ptr->label << "------------------" << endl;
    cout << "testbed_ip:       " << n_ptr->testbed_ip << endl;
    cout << "user:             " << n_ptr->user << endl;
    cout << "is_rv:            " << n_ptr->is_rv << endl;
    cout << "is_tm:            " << n_ptr->is_tm << endl;
    cout << "user:             " << n_ptr->user << endl;
    cout << "sudo:             " << n_ptr->sudo << endl;
    cout << "click_home:       " << n_ptr->click_home << endl;
    cout << "conf_home:        " << n_ptr->conf_home << endl;
    cout << "running_mode:     " << n_ptr->running_mode << endl;
    cout << "operating_system: " << n_ptr->operating_system << endl;
    cout << "internal_link_id: " << n_ptr->internal_link_id.to_string () << endl;
    cout << "lipsin id to RV   " << n_ptr->lipsin_rv.to_string () << endl;
    cout << "lipsin id to TM   " << n_ptr->lipsin_tm.to_string () << endl;

    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;
      cout << "---------CONNECTION " << c_ptr->src_label << " - " << c_ptr->dst_label << "----------" << endl;
      cout << "src_label:        " << c_ptr->src_label << endl;
      cout << "dst_label:        " << c_ptr->dst_label << endl;

      cout << "overlay_mode:     " << c_ptr->overlay_mode << endl;
      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
	cout << "src_if:           " << c_ptr->src_if << endl;
	cout << "dst_if:           " << c_ptr->dst_if << endl;
	cout << "src_mac:          " << c_ptr->src_mac << endl;
	cout << "dst_mac:          " << c_ptr->dst_mac << endl;
      } else {
	cout << "src_ip:           " << c_ptr->src_ip << endl;
	cout << "dst_ip:           " << c_ptr->dst_ip << endl;
      }
      cout << "link_id:          " << c_ptr->link_id.to_string () << endl;
      cout << "-------------------------------------------------" << endl;
    }

    BOOST_FOREACH(ns3_application_ptr app, n_ptr->ns3_applications) {
      // TODO: add stuff
    }

    cout << "-------------------------------------------------" << endl;
  }
  cout << "-------------------------------------------------" << endl;
}
