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

void
parse_configuration (boost::property_tree::ptree &pt, const string &filename, const string &format)
{
  using boost::property_tree::ptree;
  if (format.compare ("xml") == 0) {
    try {
      // Load the .xml file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      boost::property_tree::read_xml (filename, pt);
    } catch (boost::property_tree::xml_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("json") == 0) {
    try {
      // Load the .json file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_json (filename, pt);
    } catch (boost::property_tree::json_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("ini") == 0) {
    try {
      // Load the .ini file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_ini (filename, pt);
    } catch (boost::property_tree::ini_parser_error &err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    }
  } else if (format.compare ("info") == 0) {
    try {
      // Load the .info file into the property tree. If reading fails
      // (cannot open file, parse error), an exception is thrown.
      read_info (filename, pt);
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
network::load (const string &filename, const string &format)
{
  // Create an empty property tree object
  using boost::property_tree::ptree;
  ptree pt;

  /* parse configuration file */
  parse_configuration (pt, filename, format);

  /* get network parameters */
  try {
    info_id_len = pt.get<int> ("network.info_id_len");
    link_id_len = pt.get<int> ("network.link_id_len");
    is_simulation = pt.get<bool> ("network.is_simulation", false);

    user = pt.get<string> ("network.user", "unspecified");
    sudo = pt.get<bool> ("network.sudo", false);
    click_home = pt.get<string> ("network.click_home", "unspecified");
    conf_home = pt.get<string> ("network.conf_home", "unspecified");
    running_mode = pt.get<string> ("network.running_mode", "unspecified");
    operating_system = pt.get<string> ("network.operating_system", "unspecified");
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
      n_ptr->load (v.second, *this);
      pair<map<string, node_ptr>::iterator, bool> ret = nodes.insert (pair<string, node_ptr> (n_ptr->label, n_ptr));
      if (ret.second == false) {
	cerr << "Node " << n_ptr->label << " is a duplicate. Aborting..." << endl;
	exit (EXIT_FAILURE);
      } else {
	if (n_ptr->is_rv == true) {
	  if (rv_node != NULL) {
	    cerr << "Multiple RV nodes are defined. Aborting..." << endl;
	    exit (EXIT_FAILURE);
	  } else {
	    rv_node = n_ptr;
	  }
	}
	if (n_ptr->is_tm == true) {
	  if (tm_node != NULL) {
	    cerr << "Multiple TM nodes are defined. Aborting..." << endl;
	    exit (EXIT_FAILURE);
	  } else {
	    tm_node = n_ptr;
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
      c_ptr->load (v.second);

      /* insert connection in src node connection map */
      src_iter = nodes.find (c_ptr->src_label);
      if (src_iter != nodes.end ()) {
	if (nodes.find (c_ptr->dst_label) != nodes.end ()) {
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
	c_ptr->reverse (reverse_ptr);
	src_iter = nodes.find (reverse_ptr->src_label);
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

  if (is_simulation) {
    try {
      BOOST_FOREACH (ptree::value_type & v, pt.get_child ("network.applications")) {
	ns3_application_ptr a_ptr;
	a_ptr->load (v.second);
	nodes.at (a_ptr->node_label)->ns3_applications.push_back (a_ptr);
      }
    } catch (boost::property_tree::ptree_bad_data& err) {
      cerr << err.what () << endl;
      exit (EXIT_FAILURE);
    } catch (boost::property_tree::ptree_bad_path& err) {
      cerr << "No ns-3 applications are defined " << endl;
    }
  }

  if (tm_node == NULL) {
    cerr << "No TM node is defined. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }
  if (rv_node == NULL) {
    cerr << "No RV node is defined. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }
}

void
network::calculate_lid (map<string, bitvector>& link_identifiers, int index)
{
  u_int32_t bit_position;
  u_int32_t number_of_bits = (index / (link_id_len * 8)) + 1;
  bitvector link_identifier;
  do {
    link_identifier = bitvector (link_id_len * 8);
    for (int i = 0; i < number_of_bits; i++) {
      /* assign a bit in a random position */
      bit_position = rand () % (link_id_len * 8);
      link_identifier[bit_position] = true;
    }
    /* eliminate duplicate link identifiers */
  } while (link_identifiers.find (link_identifier.to_string ()) != link_identifiers.end ());

  link_identifiers.insert (pair<string, bitvector> (link_identifier.to_string (), link_identifier));
}

void
network::assign_link_ids ()
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  map<string, bitvector> link_identifiers;
  map<string, bitvector>::iterator link_identifiers_iterator;

  int lid_counter = 0;
  srand (0);

  /* how many link identifiers should I calculate? */
  int total_ids;

  /* internal link identifiers */
  total_ids = nodes.size ();

  /* link identifiers */
  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    total_ids += node_pair.second->connections.size ();
  }

  for (int i = 0; i < total_ids; i++) {
    calculate_lid (link_identifiers, i);
  }

  link_identifiers_iterator = link_identifiers.begin ();
  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_pair.second->internal_link_id = (*link_identifiers_iterator).second;
    link_identifiers_iterator++;

    BOOST_FOREACH(connection_map_pair_t connection_pair, node_pair.second->connections) {
      connection_pair.second->link_id = (*link_identifiers_iterator).second;
      link_identifiers_iterator++;
    }
  }
}

void
network::print ()
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  cout << "--------------------NETWORK----------------------" << endl;
  cout << "info_id_len:      " << info_id_len << endl;
  cout << "link_id_len:      " << link_id_len << endl;
  cout << "is_simulation:    " << is_simulation << endl;
  cout << "Topology Manager: " << tm_node->label << endl;
  cout << "Rendezvous Node:  " << rv_node->label << endl;

  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {

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

void
node::load (const boost::property_tree::ptree &pt, struct network &network)
{
  try {
    /* mandatory */
    label = pt.get<string> ("label");
    if (label.length () != network.info_id_len) {
      cerr << "Label " << label << " is not " << network.info_id_len << " bytes long. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    testbed_ip = pt.get<string> ("testbed_ip");

    /* optional - with default values */
    is_rv = pt.get<bool> ("is_rv", false);
    is_tm = pt.get<bool> ("is_tm", false);

    /* user can be set globally for the whole network */
    if (network.user.compare ("unspecified") == 0) {
      /* user was not specified globally - MUST be set on a per-node basis */
      user = pt.get<string> ("user");
    } else {
      /* if not overriden here, use the global value */
      user = pt.get<string> ("user", network.user);
    }

    /* sudo can be set globally for the whole network */
    sudo = pt.get<bool> ("user", false);
    try {
      /* if not explicitly set for this node */
      sudo = pt.get<bool> ("sudo");
    } catch (boost::property_tree::ptree_bad_path& err) {
      /* use the value (or the default one) that was set for the whole network */
      sudo = network.sudo;
    }

    /* click_home can be set globally for the whole network */
    if (network.click_home.compare ("unspecified") == 0) {
      /* click_home was not specified globally - use the provided or the default value */
      click_home = pt.get<string> ("click_home", "/home/" + user + "/click");
    } else {
      /* if not overriden here, use the global value */
      click_home = pt.get<string> ("click_home", network.click_home);
    }

    /* conf_home can be set globally for the whole network */
    if (network.conf_home.compare ("unspecified") == 0) {
      /* conf_home was not specified globally - use the provided or the default value */
      conf_home = pt.get<string> ("conf_home", "/home/" + user + "/conf");
    } else {
      /* if not overriden here, use the global value */
      conf_home = pt.get<string> ("conf_home", network.conf_home);
    }

    /* running_mode can be set globally for the whole network */
    if (network.running_mode.compare ("unspecified") == 0) {
      /* running_mode was not specified globally - use the provided or the default value */
      running_mode = pt.get<string> ("running_mode", "user");
    } else {
      /* if not overriden here, use the global value */
      running_mode = pt.get<string> ("running_mode", network.running_mode);
    }

    /* operating_system can be set globally for the whole network */
    if (network.operating_system.compare ("unspecified") == 0) {
      /* operating_system was not specified globally - use the provided or the default value */
      operating_system = pt.get<string> ("operating_system", "Linux");
    } else {
      /* if not overriden here, use the global value */
      operating_system = pt.get<string> ("operating_system", network.operating_system);
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
connection::load (const boost::property_tree::ptree &pt)
{
  try {
    /* mandatory */
    src_label = pt.get<string> ("src_label");
    dst_label = pt.get<string> ("dst_label");

    if (src_label.compare (dst_label) == 0) {
      cerr << "src_label and dst_label must not be the same. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }

    /* optional - with default value */
    overlay_mode = pt.get<string> ("overlay_mode", "Ethernet");
    if (overlay_mode.compare ("Ethernet") == 0) {
      /* mandatory */
      src_if = pt.get<string> ("src_if");
      dst_if = pt.get<string> ("dst_if");
      /* optional */
      src_mac = pt.get<string> ("src_mac", "unspecified");
      dst_mac = pt.get<string> ("dst_mac", "unspecified");
    } else if (overlay_mode.compare ("ip") == 0) {
      /* mandatory */
      src_ip = pt.get<string> ("src_ip");
      dst_ip = pt.get<string> ("dst_ip");
    } else {
      cerr << "Unrecognised overlay_mode. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* by default connections are bidirectional */
    is_bidirectional = pt.get<bool> ("is_bidirectional", "true");
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory connection parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }
}

void
connection::reverse (connection_ptr reverse_ptr)
{
  reverse_ptr->is_bidirectional = is_bidirectional;
  reverse_ptr->overlay_mode = overlay_mode;

  reverse_ptr->src_label = dst_label;
  reverse_ptr->dst_label = src_label;

  reverse_ptr->src_if = dst_if;
  reverse_ptr->dst_if = src_if;

  reverse_ptr->src_mac = dst_mac;
  reverse_ptr->dst_mac = src_mac;

  reverse_ptr->src_ip = dst_ip;
  reverse_ptr->dst_ip = src_ip;

}

void
ns3_application::load (const boost::property_tree::ptree &pt)
{

}

//string
//network::get_ip_from_label (string label)
//{
//  network_node *nn = find_node (label);
//  if (nn) return nn->testbed_ip;
//  return "";
//}

/* Values for target machines running Linux: */
#define HWADDR_LABEL   "HWaddr"
#define HWADDR_OFFSET  21

#define HWADDR_LABEL_FREEBSD   "ether"
#define HWADDR_OFFSET_FREEBSD  19

#define HWADDR_LABEL_DARWIN   "ether"
#define HWADDR_OFFSET_DARWIN  20

static void
getHwaddrLabel (const string &os, string &hwaddr_label, int &hwaddr_offset)
{
  if (os.compare ("Linux") == 0) {
    hwaddr_label = HWADDR_LABEL;
    hwaddr_offset = HWADDR_OFFSET;
  } else if (os.compare ("FreeBSD") == 0) {
    hwaddr_label = HWADDR_LABEL_FREEBSD;
    hwaddr_offset = HWADDR_OFFSET_FREEBSD;
  } else if (os.compare ("Darwin") == 0) {
    hwaddr_label = HWADDR_LABEL_DARWIN;
    hwaddr_offset = HWADDR_OFFSET_DARWIN;
  } else {
    hwaddr_label = HWADDR_LABEL;
    hwaddr_offset = HWADDR_OFFSET;
  }
}

void
network::discover_mac_addresses ()
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  map<string, string> mac_addresses;
  string line;
  FILE *fp_command;
  char response[1035];
  string command;
  string hwaddr_label;
  int hwaddr_offset;

  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_ptr n_ptr = node_pair.second;
    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;
      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {

	if (mac_addresses.find (c_ptr->src_label + c_ptr->src_if) == mac_addresses.end ()) {
	  /* get source mac address */
	  if (c_ptr->src_mac.compare ("unspecified") == 0) {
	    getHwaddrLabel (n_ptr->operating_system, hwaddr_label, hwaddr_offset);
	    if (n_ptr->sudo) {
	      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"sudo ifconfig " + c_ptr->src_if + " | grep " + hwaddr_label + "\"";
	    } else {
	      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"ifconfig " + c_ptr->src_if + " | grep " + hwaddr_label + "\"";
	    }
	    cout << command << endl;
	    fp_command = popen (command.c_str (), "r");
	    if (fp_command == NULL) {
	      cout << "Failed to run command. Aborting..." << endl;
	      pclose(fp_command);
	      exit (EXIT_FAILURE);
	    }
	    /* Read the output a line at a time and print it */
	    if (fgets (response, sizeof(response) - 1, fp_command) == NULL) {
	      cout << "Error or empty response. Aborting..." << endl;
	      pclose(fp_command);
	      exit (EXIT_FAILURE);
	    }
	    line = string (response);
	    c_ptr->src_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << c_ptr->src_mac << endl;
	    pclose(fp_command);
	  } else {
	    cout << "Interface " << c_ptr->src_if << " pre-configured with " << c_ptr->src_mac << " for connection " << c_ptr->src_label << " - " << c_ptr->dst_label << endl;
	  }
	  mac_addresses[c_ptr->src_label + c_ptr->src_if] = c_ptr->src_mac;
	} else {
	  c_ptr->src_mac = mac_addresses[c_ptr->src_label + c_ptr->src_if];
	  //cout << "I learned this mac address: " << nc->src_label << ":" << nc->src_if << " - " << mac_addresses[nc->src_label + nc->src_if] << endl;
	}

	if (mac_addresses.find (c_ptr->dst_label + c_ptr->dst_if) == mac_addresses.end ()) {
	  /*get destination mac address*/
	  if (c_ptr->dst_mac.compare ("unspecified") == 0) {
	    node_ptr dst_node_ptr = (*nodes.find (c_ptr->dst_label)).second;
	    getHwaddrLabel (dst_node_ptr->operating_system, hwaddr_label, hwaddr_offset);
	    if (dst_node_ptr->sudo) {
	      command = "ssh " + user + "@" + dst_node_ptr->testbed_ip + " -t \"sudo ifconfig " + c_ptr->dst_if + " | grep " + hwaddr_label + "\"";
	    } else {
	      command = "ssh " + user + "@" + dst_node_ptr->testbed_ip + " -t \"ifconfig " + c_ptr->dst_if + " | grep " + hwaddr_label + "\"";
	    }
	    cout << command << endl;
	    fp_command = popen (command.c_str (), "r");
	    if (fp_command == NULL) {
	      cout << "Failed to run command. Aborting..." << endl;
	      pclose(fp_command);
	      exit (EXIT_FAILURE);
	    }
	    /* Read the output a line at a time - output it. */
	    if (fgets (response, sizeof(response) - 1, fp_command) == NULL) {
	      cout << "Error or empty response. Aborting..." << endl;
	      pclose(fp_command);
	      exit (EXIT_FAILURE);
	    }
	    line = string (response);
	    c_ptr->dst_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << c_ptr->dst_mac << endl;
	    pclose(fp_command);
	  } else {
	    cout << "Interface " << c_ptr->dst_if << " pre-configured with " << c_ptr->dst_mac << " for connection " << c_ptr->src_label << " - " << c_ptr->dst_label << endl;
	  }
	  mac_addresses[c_ptr->dst_label + c_ptr->dst_if] = c_ptr->dst_mac;
	} else {
	  /* learned this MAC address before */
	  c_ptr->dst_mac = mac_addresses[c_ptr->dst_label + c_ptr->dst_if];
	}
      } else {
	cout << "Connection " << c_ptr->src_label << " - " << c_ptr->dst_label << " is over IP. Nothing to discover here..." << endl;
      }
    }
  }
}

//
//int
//findOffset (vector<string> &unique, string &str)
//{
//  for (size_t i = 0; i < unique.size (); i++) {
//    if (unique[i].compare (str) == 0) {
//      return i;
//    }
//  }
//  return -1;
//}
//
//void
//network::write_click_conf (bool montoolstub, bool dump_supp)
//{
//  ofstream click_conf;
//  ofstream write_TMFID;
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//
//    vector<string> unique_ifaces;
//    vector<string> unique_srcips;
//
//    network_node *nn = network_nodes[i];
//    click_conf.open ((write_conf + nn->label + ".conf").c_str ());
//    write_TMFID.open ((write_conf + nn->label + "_TMFID.txt").c_str ());
//    if (montoolstub && (nn->running_mode.compare ("user") == 0)) {
//      click_conf << "require(blackadder); \n\nControlSocket(\"TCP\",55000);\n\n " << endl << endl;
//    } else {
//      click_conf << "require(blackadder);" << endl << endl;
//    }
//    /*Blackadder Elements First*/
//    click_conf << "globalconf::GlobalConf(MODE " << overlay_mode << ", NODEID " << nn->label << "," << endl;
//    click_conf << "DEFAULTRV " << nn->fid_to_rv.to_string () << "," << endl;
//    click_conf << "TMFID     " << nn->fid_to_tm.to_string () << "," << endl;
//    write_TMFID << nn->fid_to_tm.to_string () << endl;
//    click_conf << "iLID      " << nn->internal_lid.to_string () << ");" << endl << endl;
//
//    click_conf << "localRV::LocalRV(globalconf);" << endl;
//    click_conf << "netlink::Netlink();" << endl << "tonetlink::ToNetlink(netlink);" << endl << "fromnetlink::FromNetlink(netlink);" << endl << endl;
//    click_conf << "proxy::LocalProxy(globalconf);" << endl << endl;
//    click_conf << "fw::Forwarder(globalconf," << nn->connections.size () << "," << endl;
//    for (size_t j = 0; j < nn->connections.size (); j++) {
//      network_connection *nc = nn->connections[j];
//      int offset;
//      if (overlay_mode.compare ("mac") == 0) {
//
//	// Default MAC behaviour
//	if ((offset = findOffset (unique_ifaces, nc->src_if)) == -1) {
//	  unique_ifaces.push_back (nc->src_if);
//	  click_conf << unique_ifaces.size () << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string ();
//	} else {
//	  click_conf << offset + 1 << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string ();
//	}
//
//	// Do not ignore duplicates and use a new port!
//      } else {
//	if ((offset = findOffset (unique_srcips, nc->src_ip)) == -1) {
//	  unique_srcips.push_back (nc->src_ip);
//	  //cout << "PUSHING BACK " << nc->src_ip << endl;
//	  //cout << unique_srcips.size() << endl;
//	  click_conf << unique_srcips.size () << "," << nc->src_ip << "," << nc->dst_ip << "," << nc->lid.to_string ();
//	} else {
//	  click_conf << offset + 1 << "," << nc->src_ip << "," << nc->dst_ip << "," << nc->lid.to_string ();
//	}
//      }
//      if (j < nn->connections.size () - 1) {
//	/* We assume that all if and else clauses above print a line */
//	click_conf << "," << endl;
//      }
//    }
//    click_conf << ");" << endl << endl;
//    if ((overlay_mode.compare ("mac") == 0)) {
//      for (size_t j = 0; j < unique_ifaces.size (); j++) {
//	click_conf << "tsf" << j << "::ThreadSafeQueue(1000);" << endl;
//	if (nn->running_mode.compare ("user") == 0) {
//	  click_conf << "fromdev" << j << "::FromDevice(" << unique_ifaces[j] << ");" << endl << "todev" << j << "::ToDevice(" << unique_ifaces[j] << ");" << endl;
//	} else {
//	  click_conf << "fromdev" << j << "::FromDevice(" << unique_ifaces[j] << ", BURST 8);" << endl << "todev" << j << "::ToDevice(" << unique_ifaces[j] << ", BURST 8);" << endl;
//	}
//      }
//      /*Necessary Click Elements*/
//    } else {
//      /*raw sockets here*/
//      click_conf << "tsf" << "::ThreadSafeQueue(1000);" << endl;
//      if (nn->running_mode.compare ("user") == 0) {
//	click_conf << "rawsocket" << "::RawSocket(UDP, 55555)" << endl;
//	//click_conf << "classifier::IPClassifier(dst udp port 55000 and src udp port 55000)" << endl;
//      } else {
//	cerr << "Something is wrong...I should not build click config using raw sockets for node " << nn->label << "that will run in kernel space" << endl;
//      }
//    }
//
//    /*Now link all the elements appropriately*/
//    click_conf << endl << endl << "proxy[0]->tonetlink;" << endl << "fromnetlink->[0]proxy;" << endl << "localRV[0]->[1]proxy[1]->[0]localRV;" << endl << "proxy[2]-> [0]fw[0] -> [2]proxy;" << endl;
//    if ((overlay_mode.compare ("mac") == 0)) {
//      for (size_t j = 0; j < unique_ifaces.size (); j++) {
//	if (nn->running_mode.compare ("kernel") == 0) {
//	  click_conf << endl << "classifier" << j << "::Classifier(12/080a,-);" << endl;
//	  if (montoolstub && j == 0 && (nn->running_mode.compare ("user") == 0)) {
//	    click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->outc::Counter()->todev" << j << ";" << endl;
//	    click_conf << "fromdev" << j << "->classifier" << j << "[0]->inc::Counter()  -> [" << (j + 1) << "]fw;" << endl;
//	  } else {
//	    click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->todev" << j << ";" << endl;
//	    click_conf << "fromdev" << j << "->classifier" << j << "[0]->[" << (j + 1) << "]fw;" << endl;
//	  }
//	  click_conf << "classifier" << j << "[1]->ToHost()" << endl;
//	} else {
//	  click_conf << endl << "classifier" << j << "::Classifier(12/080a);" << endl;
//	  if (montoolstub && j == 0 && (nn->running_mode.compare ("user") == 0)) {
//	    click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->outc::Counter()->todev" << j << ";" << endl;
//	    click_conf << "fromdev" << j << "->classifier" << j << "[0]->inc::Counter()  -> [" << (j + 1) << "]fw;" << endl;
//	  } else {
//	    click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->todev" << j << ";" << endl;
//	    click_conf << "fromdev" << j << "->classifier" << j << "[0]->[" << (j + 1) << "]fw;" << endl;
//	  }
//	}
//      }
//    } else {
//      /*raw sockets here*/
//      if (montoolstub) {
//	click_conf << "fw[1] -> tsf -> outc::Counter() -> rawsocket -> IPClassifier(dst udp port 55555 and src udp port 55555)[0]-> inc::Counter()  -> [1]fw" << endl;
//      } else {
//	click_conf << "fw[1] ->  tsf -> rawsocket -> IPClassifier(dst udp port 55555 and src udp port 55555)[0] -> [1]fw" << endl;
//      }
//    }
//    if (dump_supp && nn->is_rv && nn->running_mode.compare ("user") == 0) {
//      click_conf << "\ncs :: ControlSocket(TCP, 55500);" << endl;
//    }
//    click_conf.close ();
//    write_TMFID.close ();
//  }
//}
//
//void
//network::scp_tm_conf (string TM_conf)
//{
//  FILE *scp_command;
//  string command;
//  command = "scp " + write_conf + TM_conf + " " + user + "@" + TM_node->testbed_ip + ":" + write_conf;
//  cout << command << endl;
//  scp_command = popen (command.c_str (), "r");
//  /* close */
//  pclose (scp_command);
//}
//
//void
//network::scp_click_conf ()
//{
//  FILE *scp_command;
//  string command, fidtm_cmd;
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    command = "scp " + write_conf + nn->label + ".conf" + " " + user + "@" + nn->testbed_ip + ":" + write_conf;
//    fidtm_cmd = "scp " + write_conf + nn->label + "_TMFID.txt" + " " + user + "@" + nn->testbed_ip + ":" + write_conf;
//    cout << command << endl;
//    scp_command = popen (command.c_str (), "r");
//    if (scp_command == NULL) {
//      cerr << "Failed to scp click file to node " << nn->label << endl;
//    }
//    pclose (scp_command);
//    cout << fidtm_cmd << endl;
//    scp_command = popen (fidtm_cmd.c_str (), "r");
//    if (scp_command == NULL) {
//      cerr << "Failed to scp TMFID file to node " << nn->label << endl;
//    }
//    /* close */
//    pclose (scp_command);
//  }
//}
//
//void
//network::start_click ()
//{
//  FILE *ssh_command;
//  string command;
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    /*kill click first both from kernel and user space*/
//    if (sudo) {
//      command = "ssh " + user + "@" + nn->testbed_ip + " -t \"sudo pkill -9 click\"";
//    } else {
//      command = "ssh " + user + "@" + nn->testbed_ip + " -t \"pkill -9 click\"";
//    }
//    cout << command << endl;
//    ssh_command = popen (command.c_str (), "r");
//    if (ssh_command == NULL) {
//      cerr << "Failed to stop click at node " << nn->label << endl;
//    }
//    pclose (ssh_command);
//    if (sudo) {
//      command = "ssh " + user + "@" + nn->testbed_ip + " -t \"sudo " + click_home + "sbin/click-uninstall\"";
//    } else {
//      command = "ssh " + user + "@" + nn->testbed_ip + " -t \"" + click_home + "sbin/click-uninstall \"";
//    }
//    cout << command << endl;
//    ssh_command = popen (command.c_str (), "r");
//    if (ssh_command == NULL) {
//      cerr << "Failed to stop click at node " << nn->label << endl;
//    }
//    pclose (ssh_command);
//    /*now start click*/
//    if (nn->running_mode.compare ("user") == 0) {
//      if (sudo) {
//	command = "ssh " + user + "@" + nn->testbed_ip + " \"sudo " + click_home + "bin/click " + write_conf + nn->label + ".conf > /dev/null 2>&1 &\"";
//      } else {
//	command = "ssh " + user + "@" + nn->testbed_ip + " \"" + click_home + "bin/click " + write_conf + nn->label + ".conf > /dev/null 2>&1 &\"";
//      }
//      cout << command << endl;
//      ssh_command = popen (command.c_str (), "r");
//      if (ssh_command == NULL) {
//	cerr << "Failed to start click at node " << nn->label << endl;
//      }
//      pclose (ssh_command);
//    } else {
//      if (sudo) {
//	command = "ssh " + user + "@" + nn->testbed_ip + " \"sudo " + click_home + "sbin/click-install " + write_conf + nn->label + ".conf > /dev/null 2>&1 &\"";
//      } else {
//	command = "ssh " + user + "@" + nn->testbed_ip + " \"" + click_home + "sbin/click-install " + write_conf + nn->label + ".conf > /dev/null 2>&1 &\"";
//      }
//      cout << command << endl;
//      ssh_command = popen (command.c_str (), "r");
//      if (ssh_command == NULL) {
//	cerr << "Failed to start click at node " << nn->label << endl;
//      }
//      pclose (ssh_command);
//    }
//  }
//}
//
//void
//network::start_tm ()
//{
//  FILE *ssh_command;
//  string command;
//  /*kill the topology manager first*/
//  command = "ssh " + user + "@" + TM_node->testbed_ip + " -t \"pkill -9 tm\"";
//  cout << command << endl;
//  ssh_command = popen (command.c_str (), "r");
//  if (ssh_command == NULL) {
//    cerr << "Failed to stop Topology Manager at node " << TM_node->label << endl;
//  }
//  pclose (ssh_command);
//  /*now start the TM*/
//  command = "ssh " + user + "@" + TM_node->testbed_ip + " -t \"/home/" + user + "/blackadder/TopologyManager/tm " + write_conf + "topology.graphml > /dev/null 2>&1 &\"";
//  cout << command << endl;
//  ssh_command = popen (command.c_str (), "r");
//  if (ssh_command == NULL) {
//    cerr << "Failed to start Topology Manager at node " << TM_node->label << endl;
//  }
//  pclose (ssh_command);
//}

//string
//network::get_next_mac_address ()
//{
//  uint8_t m_address[6];
//  string mac_str;
//  stringstream ss;
//  m_address[0] = (id >> 40) & 0xff;
//  m_address[1] = (id >> 32) & 0xff;
//  m_address[2] = (id >> 24) & 0xff;
//  m_address[3] = (id >> 16) & 0xff;
//  m_address[4] = (id >> 8) & 0xff;
//  m_address[5] = (id >> 0) & 0xff;
//  id++;
//  ss.setf (ios::hex, ios::basefield);
//
//  ss.fill ('0');
//  for (uint8_t i = 0; i < 5; i++) {
//    ss << setw (2) << (uint32_t) m_address[i] << ":";
//  }
//  // Final byte not suffixed by ":"
//  ss << setw (2) << (uint32_t) m_address[5];
//  ss.setf (ios::dec, ios::basefield);
//  ss.fill (' ');
//  mac_str = ss.str ();
//  return mac_str;
//}

/* probably the most inefficient way of doing it!! */
void
network::assign_mac_addresses ()
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_ptr n_ptr = node_pair.second;

    //cout << "Looking at Node " << n_ptr->label << ", Device Offset " << n_ptr->device_offset << endl;

    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;

//      cout << "Looking at Connection " << nc->src_label << " -> " << nc->dst_label << endl;
//
//      /*check if I have already considered this connection (if it is bidirectional) before*/
//      if ((nc->src_if.empty ()) && (nc->dst_if.empty ())) {
//	stringstream out1;
//	out1 << nn->device_offset;
//	nc->src_if = "eth" + out1.str ();
//	nc->src_mac = get_next_mac_address ();
//	stringstream out2;
//	network_node *neighbour = get_node (nc->dst_label);
//	out2 << neighbour->device_offset;
//	nc->dst_if = "eth" + out2.str ();
//	nc->dst_mac = get_next_mac_address ();
//	cout << "Assigning " << nn->label << ", device offset " << nn->device_offset << ", device name " << nc->src_if << ", MAC Address " << nc->src_mac << endl;
//	cout << "Assigning " << neighbour->label << ", device offset " << neighbour->device_offset << ", device name " << nc->dst_if << ", MAC Address " << nc->dst_mac << endl;
//	nn->device_offset++;
//	neighbour->device_offset++;
//	/*find the connection from the neighbour node and assign the same interfaces*/
//	network_connection *neighbourConnection = neighbour->getConnection (nc->dst_label, nc->src_label);
//	neighbourConnection->src_if = nc->dst_if;
//	neighbourConnection->dst_if = nc->src_if;
//	neighbourConnection->src_mac = nc->dst_mac;
//	neighbourConnection->dst_mac = nc->src_mac;
//      } else {
//	cout << "Already checked that connection" << endl;
//	cout << "device name " << nc->src_if << ", MAC Address " << nc->src_mac << endl;
//	cout << "device name " << nc->dst_if << ", MAC Address " << nc->dst_mac << endl;
//      }
    }
  }
}

//void
//network::create_ns3_code ()
//{
//  ofstream ns3_code;
//  ns3_code.open ((write_conf + "topology.cpp").c_str ());
//  /*First write all required stuff before starting to build the actual topology*/
//  ns3_code << "#include <ns3/core-module.h>" << endl;
//  ns3_code << "#include <ns3/network-module.h>" << endl;
//  ns3_code << "#include <ns3/point-to-point-module.h>" << endl;
//  ns3_code << "#include <ns3/blackadder-module.h>" << endl;
//  ns3_code << "using namespace ns3;" << endl;
//  ns3_code << "NS_LOG_COMPONENT_DEFINE(\"topology\");" << endl;
//  ns3_code << "int main(int argc, char *argv[]) {" << endl;
//  /*create all nodes now*/
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    ns3_code << "   Ptr<Node> node" << i << " = CreateObject<Node>();" << endl;
//    stringstream out2;
//    out2 << "node" << i;
//    label_to_node_name.insert (pair<string, string> (nn->label, out2.str ()));
//    for (size_t j = 0; j < nn->connections.size (); j++) {
//      network_connection *nc = nn->connections[j];
//      ns3_code << "   Ptr<PointToPointNetDevice> dev" << i << "_" << j << " = Create<PointToPointNetDevice>();" << endl;
//      ns3_code << "   dev" << i << "_" << j << "->SetAddress (Mac48Address(\"" << nc->src_mac << "\"));" << endl;
//      ns3_code << "   dev" << i << "_" << j << "->SetDataRate (DataRate(\"" << nc->rate << "\"));" << endl;
//      ns3_code << "   dev" << i << "_" << j << "->SetMtu (" << nc->mtu << ");" << endl;
//      stringstream out1;
//      out1 << "dev" << i << "_" << j;
//      address_to_device_name.insert (pair<string, string> (nc->src_mac, out1.str ()));
//      ns3_code << "   node" << i << "->AddDevice(dev" << i << "_" << j << ");" << endl;
//      /*The type of queue can be a parameter in the initial topology file that is parsed (along with its attributes)*/
//      ns3_code << "   Ptr<DropTailQueue> queue" << i << "_" << j << " = CreateObject<DropTailQueue > ();" << endl;
//      ns3_code << "   dev" << i << "_" << j << "->SetQueue(queue" << i << "_" << j << ");" << endl;
//    }
//    ns3_code << endl;
//  }
//  int channel_counter = 0;
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    for (size_t j = 0; j < nn->connections.size (); j++) {
//      network_connection *nc = nn->connections[j];
//      if ((NS3devices_in_connections.find (nc->src_mac) == NS3devices_in_connections.end ()) && (NS3devices_in_connections.find (nc->dst_mac) == NS3devices_in_connections.end ())) {
//	ns3_code << "   Ptr<PointToPointChannel> channel" << channel_counter << " = CreateObject<PointToPointChannel>();" << endl;
//	ns3_code << "   channel" << channel_counter << "->SetAttribute(\"Delay\", StringValue(\"" << nc->delay << "\"));" << endl;
//	ns3_code << "   " << (*address_to_device_name.find (nc->src_mac)).second << "->Attach(channel" << channel_counter << ");" << endl;
//	ns3_code << "   " << (*address_to_device_name.find (nc->dst_mac)).second << "->Attach(channel" << channel_counter << ");" << endl;
//	NS3devices_in_connections.insert (nc->src_mac);
//	NS3devices_in_connections.insert (nc->dst_mac);
//	channel_counter++;
//	ns3_code << endl;
//      }
//    }
//  }
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    ns3_code << "   Ptr<ClickBridge> click" << i << " = CreateObject<ClickBridge > ();" << endl;
//    ns3_code << "   node" << i << "->AggregateObject(click" << i << ");" << endl;
//    ns3_code << "   click" << i << "->SetClickFile(\"" << write_conf << "/" << nn->label << ".conf\");" << endl;
//    ns3_code << "   Ptr<ServiceModel> servModel" << i << " = CreateObject<ServiceModel > (); " << endl;
//    ns3_code << "   node" << i << "->AggregateObject(servModel" << i << "); " << endl;
//  }
//  ns3_code << endl;
//
//  /*Start the topology manager application to the respective network node*/
//  ns3_code << "   Ptr<TopologyManager> tm = CreateObject<TopologyManager > ();" << endl;
//  ns3_code << "   tm->SetStartTime(Seconds(0.)); " << endl;
//  ns3_code << "   tm->SetAttribute(\"Topology\", StringValue(\"" << write_conf << "topology.graphml\"));" << endl;
//  ns3_code << "   " << (*label_to_node_name.find (TM_node->label)).second << "->AddApplication(tm); " << endl;
//  ns3_code << endl;
//  /*add applications and attributes*/
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    network_node *nn = network_nodes[i];
//    for (size_t j = 0; j < nn->applications.size (); j++) {
//      ns3_application *app = nn->applications[j];
//      ns3_code << "   Ptr<" << app->name << "> app" << i << "_" << j << " = CreateObject<" << app->name << "> ();" << endl;
//      ns3_code << "   " << "app" << i << "_" << j << "->SetStartTime(Seconds(" << app->start << ")); " << endl;
//      ns3_code << "   " << "app" << i << "_" << j << "->SetStopTime(Seconds(" << app->stop << ")); " << endl;
//      for (size_t k = 0; k < app->attributes.size (); k++) {
//	ns3_application_attribute *attr = app->attributes[k];
//	ns3_code << "   " << "app" << i << "_" << j << "->SetAttribute(\"" << attr->name << "\", " << attr->value << ");" << endl;
//      }
//      ns3_code << "   node" << i << "->AddApplication(app" << i << "_" << j << ");" << endl;
//      ns3_code << endl;
//    }
//  }
//  ns3_code << "   Simulator::Run();" << endl;
//  ns3_code << "   Simulator::Destroy();" << endl;
//  ns3_code << "   return 0;" << endl;
//
//  ns3_code << "}" << endl;
//  ns3_code.close ();
//}
//void
//network::write_ns3_click_conf ()
//{
//  ofstream click_conf;
//  for (size_t i = 0; i < network_nodes.size (); i++) {
//    vector<string> unique_ifaces;
//    network_node *nn = network_nodes[i];
//    click_conf.open ((write_conf + nn->label + ".conf").c_str ());
//    /*Blackadder Elements First*/
//    click_conf << "globalconf::GlobalConf(MODE mac, NODEID " << nn->label << "," << endl;
//    click_conf << "DEFAULTRV " << nn->fid_to_rv.to_string () << "," << endl;
//    click_conf << "TMFID     " << nn->fid_to_tm.to_string () << "," << endl;
//    click_conf << "iLID      " << nn->internal_lid.to_string () << ");" << endl << endl;
//    click_conf << "localRV::LocalRV(globalconf);" << endl;
//    click_conf << "toApps::ToSimDevice(tap0);" << endl << "fromApps::FromSimDevice(tap0);" << endl;
//    click_conf << "proxy::LocalProxy(globalconf);" << endl << endl;
//    click_conf << "fw::Forwarder(globalconf," << nn->connections.size () << "," << endl;
//    for (size_t j = 0; j < nn->connections.size (); j++) {
//      network_connection *nc = nn->connections[j];
//      int offset;
//      if ((offset = findOffset (unique_ifaces, nc->src_if)) == -1) {
//	unique_ifaces.push_back (nc->src_if);
//	if (j == nn->connections.size () - 1) {
//	  click_conf << unique_ifaces.size () << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string () << ");" << endl << endl;
//	} else {
//	  click_conf << unique_ifaces.size () << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string () << "," << endl;
//	}
//      } else {
//	if (j == nn->connections.size () - 1) {
//	  click_conf << offset + 1 << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string () << ");" << endl << endl;
//	} else {
//	  click_conf << offset + 1 << "," << nc->src_mac << "," << nc->dst_mac << "," << nc->lid.to_string () << "," << endl;
//	}
//      }
//    }
//    for (size_t j = 0; j < unique_ifaces.size (); j++) {
//      click_conf << "tsf" << j << "::ThreadSafeQueue(1000);" << endl;
//      click_conf << "fromsimdev" << j << "::FromSimDevice(" << unique_ifaces[j] << ");" << endl << "tosimdev" << j << "::ToSimDevice(" << unique_ifaces[j] << ");" << endl;
//    }
//    /*Now link all the elements appropriately*/
//    click_conf << "proxy[0]->toApps;" << endl << "fromApps->[0]proxy;" << endl << "localRV[0]->[1]proxy[1]->[0]localRV;" << endl << "proxy[2]-> [0]fw[0] -> [2]proxy;" << endl;
//    for (size_t j = 0; j < unique_ifaces.size (); j++) {
//      click_conf << "fw[" << (j + 1) << "]->tsf" << j << "->tosimdev" << j << ";" << endl;
//      click_conf << "fromsimdev" << j << "->[" << (j + 1) << "]fw;" << endl;
//    }
//    click_conf.close ();
//  }
//}
