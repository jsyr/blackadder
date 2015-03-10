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

/* a global variable: the folder where all configuration files will be stored before deploying blackadder */
string tmp_conf_folder;

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

  /* read the tmp folder where all configuration will be stored before is sent to blackadder nodes */
  /* not that the machine from which we deploy may not be part of the deployment itself */
  /* so a global variable will be set - it is not part of the network struct */
  tmp_conf_folder = pt.get<string> ("tmp_conf_folder");
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

/* Values for target machines */
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
	      pclose (fp_command);
	      exit (EXIT_FAILURE);
	    }
	    /* Read the output a line at a time and print it */
	    if (fgets (response, sizeof(response) - 1, fp_command) == NULL) {
	      cout << "Error or empty response. Aborting..." << endl;
	      pclose (fp_command);
	      exit (EXIT_FAILURE);
	    }
	    line = string (response);
	    c_ptr->src_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << c_ptr->src_mac << endl;
	    pclose (fp_command);
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
	      pclose (fp_command);
	      exit (EXIT_FAILURE);
	    }
	    /* Read the output a line at a time - output it. */
	    if (fgets (response, sizeof(response) - 1, fp_command) == NULL) {
	      cout << "Error or empty response. Aborting..." << endl;
	      pclose (fp_command);
	      exit (EXIT_FAILURE);
	    }
	    line = string (response);
	    c_ptr->dst_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << c_ptr->dst_mac << endl;
	    pclose (fp_command);
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

void
network::write_click_conf ()
{
  /* Click directory path */
  path tmp_conf_path (tmp_conf_folder);

  if (!boost::filesystem::exists (tmp_conf_path)) {
    cout << "creating directory " << tmp_conf_path << " to store configuration files before deploying blackadder" << endl;
    boost::filesystem::create_directory (tmp_conf_path);
  }

  if (!boost::filesystem::is_directory (tmp_conf_path)) {
    cerr << tmp_conf_path << " is a regular file. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  /* all Click configuration files will be store locally under /tmp */
  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_ptr n_ptr = node_pair.second;

    map<string, int> unique_ifaces;
    map<string, int> unique_srcips;
    map<string, int>::iterator unique_ifaces_iterator;
    set<string> link_local_entries;

    int unique_iface_index = 1;
    int unique_ip_index = 1;

    path click_conf_file_path (tmp_conf_path / (n_ptr->label + ".conf"));

    cout << "writing Click configuration for blackadder node " << n_ptr->label << " at " << click_conf_file_path << endl;

    if (boost::filesystem::exists (click_conf_file_path)) {
      boost::filesystem::remove (click_conf_file_path);
    }

    boost::filesystem::ofstream click_conf;

    click_conf.open (click_conf_file_path);

    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;

      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
	if (unique_ifaces.find (c_ptr->src_if) == unique_ifaces.end ()) {
	  unique_ifaces.insert (std::pair<string, int> (c_ptr->src_if, unique_iface_index));
	  unique_iface_index++;
	}
      } else if (c_ptr->overlay_mode.compare ("IP") == 0) {
	if (unique_srcips.find (c_ptr->src_ip) == unique_srcips.end ()) {
	  unique_srcips.insert (std::pair<string, int> (c_ptr->src_ip, unique_ip_index));
	  unique_ip_index++;
	}
      } else {
	cout << "Unknown overlay mode. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
    }

    click_conf << "require(blackadder);" << endl << endl;
    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      if (n_ptr->running_mode.compare ("kernel") == 0) {
	click_conf << "network_classifier" << (*unique_ifaces_iterator).second << "::Classifier(12/080a,-);" << endl;
      } else {
	click_conf << "network_classifier" << (*unique_ifaces_iterator).second << "::Classifier(12/080a);" << endl;
      }
    }
    click_conf << "protocol_classifier::Classifier(0/00,0/01,-);" << endl;
    click_conf << "notification_classifier::Classifier(0/01000000, 0/FF000000, -);" << endl;
    click_conf << "dispatcher_queue::ThreadSafeQueue();" << endl;
    click_conf << "to_user_queue::ThreadSafeQueue();" << endl;
    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      click_conf << "to_network_queue" << (*unique_ifaces_iterator).second << "::ThreadSafeQueue();" << endl;
    }
    if (unique_srcips.size () > 0) {
      click_conf << "to_network_queue" << unique_ifaces.size () + 1 << "::ThreadSafeQueue();" << endl;
    }

    click_conf << "from_user::FromUser();" << endl;
    click_conf << "to_user::ToUser(from_user);" << endl << endl;
    click_conf << "dispatcher::Dispatcher(NODEID " << n_ptr->label << ",DEFAULTRV " << n_ptr->lipsin_rv.to_string () << ");" << endl << endl;
    click_conf << "rv::RV(NODEID " << n_ptr->label << ",TMFID " << n_ptr->lipsin_tm.to_string () << ");" << endl << endl;
    if (unique_srcips.size () > 0) {
      click_conf << "fw::Forwarder(" << unique_ifaces.size () + 1 << "," << endl;
    } else {
      click_conf << "fw::Forwarder(" << unique_ifaces.size () << "," << endl;
    }

    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      click_conf << (*unique_ifaces_iterator).second << ",MAC," << endl;
    }
    if (unique_srcips.size () > 0) {
      click_conf << unique_ifaces.size () + 1 << ",IP," << endl;
    }

    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;
      /*1 is the link-local forwarding strategy*/
      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
	if (link_local_entries.find (c_ptr->src_mac + c_ptr->dst_mac) == link_local_entries.end ()) {
	  link_local_entries.insert (c_ptr->src_mac + c_ptr->dst_mac);
	}
      } else if (c_ptr->overlay_mode.compare ("IP") == 0) {
	if (link_local_entries.find (c_ptr->src_ip + c_ptr->dst_ip) == link_local_entries.end ()) {
	  link_local_entries.insert (c_ptr->src_ip + c_ptr->dst_ip);
	}
      }
    }
    click_conf << n_ptr->connections.size () + link_local_entries.size () + 1 << "," << endl;
    /*Add the Internal Link for Lipsin based strategy*/
    click_conf << "2,0,INTERNAL," << n_ptr->internal_link_id.to_string () << "," << endl;

    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;

      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
	/* 2 is the LIPSIN based forwarding strategy */
	click_conf << "2," << (*unique_ifaces.find (c_ptr->src_if)).second << ",MAC," << c_ptr->src_mac << "," << c_ptr->dst_mac << "," << c_ptr->link_id.to_string () << "," << endl;
      } else if (c_ptr->overlay_mode.compare ("IP") == 0) {
	click_conf << "2," << unique_ifaces.size () + 1 << ",IP," << c_ptr->src_ip << "," << c_ptr->dst_ip << "," << c_ptr->link_id.to_string () << "," << endl;
      } else {
	cout << "Unknown overlay mode. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
    }

    /*now add the link strategy entries*/
    link_local_entries.clear ();
    BOOST_FOREACH(connection_map_pair_t connection_pair, n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;

      /* 1 is the link-local forwarding strategy */
      if (c_ptr->overlay_mode.compare ("Ethernet") == 0) {
	if (link_local_entries.find (c_ptr->src_mac + c_ptr->dst_mac) == link_local_entries.end ()) {
	  link_local_entries.insert (c_ptr->src_mac + c_ptr->dst_mac);
	  click_conf << "1," << (*unique_ifaces.find (c_ptr->src_if)).second << ",MAC," << c_ptr->src_mac << "," << c_ptr->dst_mac << "," << endl;
	}
      } else if (c_ptr->overlay_mode.compare ("IP") == 0) {
	if (link_local_entries.find (c_ptr->src_ip + c_ptr->dst_ip) == link_local_entries.end ()) {
	  link_local_entries.insert (c_ptr->src_ip + c_ptr->dst_ip);
	  click_conf << "1," << unique_ifaces.size () + 1 << ",IP," << c_ptr->src_ip << "," << c_ptr->dst_ip << "," << endl;
	} else {
	  cout << "Unknown overlay mode. Aborting..." << endl;
	  exit (EXIT_FAILURE);
	}
      }
    }

    click_conf << ");" << endl << endl;

    /*add network devices*/
    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      if (n_ptr->running_mode.compare ("user") == 0) {
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "::FromDevice(" << (*unique_ifaces_iterator).first << ",METHOD LINUX);" << endl << "todev" << (*unique_ifaces_iterator).second
	    << "::ToDevice(" << (*unique_ifaces_iterator).first << ",METHOD LINUX);" << endl;
      } else {
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "::FromDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl << "todev" << (*unique_ifaces_iterator).second
	    << "::ToDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl;
	click_conf << "tohost::ToHost();" << endl;
      }
    }
    if (unique_srcips.size () > 0) {
      if (n_ptr->running_mode.compare ("user") == 0) {
	click_conf << "rawsocket" << "::RawSocket(UDP, 55555);" << endl;
      } else {
	cerr << "Something is wrong...I should not build click config using raw sockets for node " << n_ptr->label << "that will run in kernel space. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
    }
    click_conf << endl;
    /*Now link all the elements appropriately*/
    click_conf << "from_user->protocol_classifier;" << endl;
    click_conf << "protocol_classifier[0]-> Strip(1)->dispatcher_queue;" << endl;
    click_conf << "protocol_classifier[1]-> Strip(1)-> Print(LABEL \"Fountain Codes : \")-> Discard;" << endl;
    click_conf << "protocol_classifier[2]-> Strip(1)-> Print(LABEL \"Unknown Protocol : \")-> Discard;" << endl;
    click_conf << "dispatcher_queue->Unqueue()->[0]dispatcher;" << endl;
    click_conf << "dispatcher[0]->notification_classifier;" << endl;
    click_conf << "notification_classifier[0]->Strip(4)->Print(LABEL \"To Fountain Element\")-> Discard;" << endl;
    click_conf << "notification_classifier[1]->Strip(4)->rv;" << endl;
    click_conf << "notification_classifier[2]->to_user_queue->Unqueue->to_user;" << endl;
    click_conf << "rv->protocol_classifier;" << endl;
    click_conf << "dispatcher[1]-> [0]fw[0] -> [1]dispatcher;" << endl;

    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      if (n_ptr->running_mode.compare ("kernel") == 0) {
	click_conf << "fw[" << (*unique_ifaces_iterator).second << "]->to_network_queue" << (*unique_ifaces_iterator).second << "->todev" << (*unique_ifaces_iterator).second << ";" << endl;
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "->network_classifier" << (*unique_ifaces_iterator).second << "[0]->[" << (*unique_ifaces_iterator).second << "]fw;" << endl;
	click_conf << "network_classifier" << (*unique_ifaces_iterator).second << "[1]->tohost" << endl;
      } else {
	click_conf << "fw[" << (*unique_ifaces_iterator).second << "]->to_network_queue" << (*unique_ifaces_iterator).second << "->todev" << (*unique_ifaces_iterator).second << ";" << endl;
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "->network_classifier" << (*unique_ifaces_iterator).second << "[0]->[" << (*unique_ifaces_iterator).second << "]fw;" << endl;
      }
    }
    if (unique_srcips.size () > 0) {
      click_conf << "fw[" << unique_ifaces.size () + 1 << "] ->  to_network_queue" << unique_ifaces.size () + 1 << " -> rawsocket" << endl;
      click_conf << "rawsocket -> IPClassifier(dst udp port 55555 and src udp port 55555)[0] -> [" << unique_ifaces.size () + 1 << "]fw" << endl;
    }
    click_conf.close ();
  }
}

void
network::scp_click_conf ()
{
  FILE *scp_command;
  string command;

  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_ptr n_ptr = node_pair.second;

    command = "scp " + n_ptr->conf_home + n_ptr->label + ".conf" + " " + n_ptr->user + "@" + n_ptr->testbed_ip + ":" + n_ptr->conf_home;

    cout << command << endl;

    scp_command = popen (command.c_str (), "r");

    if (scp_command == NULL) {
      cerr << "Failed to scp click configuration file to node " << n_ptr->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* close */
    pclose (scp_command);
  }
}

void
network::start_click ()
{
  FILE *ssh_command;
  string command;

  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    node_ptr n_ptr = node_pair.second;
    /*kill click first both from kernel and user space*/
    if (sudo) {
      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"sudo pkill -9 click\"";
    } else {
      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"pkill -9 click\"";
    }
    cout << command << endl;
    ssh_command = popen (command.c_str (), "r");
    if (ssh_command == NULL) {
      cerr << "Failed to stop click at node " << n_ptr->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    pclose (ssh_command);
    if (sudo) {
      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"sudo " + click_home + "sbin/click-uninstall\"";
    } else {
      command = "ssh " + user + "@" + n_ptr->testbed_ip + " -t \"" + click_home + "sbin/click-uninstall \"";
    }
    cout << command << endl;
    ssh_command = popen (command.c_str (), "r");
    if (ssh_command == NULL) {
      cerr << "Failed to stop click at node " << n_ptr->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    pclose (ssh_command);
    /*now start click*/
    if (n_ptr->running_mode.compare ("user") == 0) {
      if (sudo) {
	command = "ssh " + user + "@" + n_ptr->testbed_ip + " \"sudo " + click_home + "bin/click " + n_ptr->conf_home + n_ptr->label + ".conf > /dev/null 2>&1 &\"";
      } else {
	command = "ssh " + user + "@" + n_ptr->testbed_ip + " \"" + click_home + "bin/click " + n_ptr->conf_home + n_ptr->label + ".conf > /dev/null 2>&1 &\"";
      }
      cout << command << endl;
      ssh_command = popen (command.c_str (), "r");
      if (ssh_command == NULL) {
	cerr << "Failed to start click at node " << n_ptr->label << ". Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
      pclose (ssh_command);
    } else {
      if (sudo) {
	command = "ssh " + user + "@" + n_ptr->testbed_ip + " \"sudo " + click_home + "sbin/click-install " + n_ptr->conf_home + n_ptr->label + ".conf > /dev/null 2>&1 &\"";
      } else {
	command = "ssh " + user + "@" + n_ptr->testbed_ip + " \"" + click_home + "sbin/click-install " + n_ptr->conf_home + n_ptr->label + ".conf > /dev/null 2>&1 &\"";
      }
      cout << command << endl;
      ssh_command = popen (command.c_str (), "r");
      if (ssh_command == NULL) {
	cerr << "Failed to start click at node " << n_ptr->label << ". Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
      pclose (ssh_command);
    }
  }
}

void
network::scp_tm_conf (string tm_conf)
{
  FILE *scp_command;
  string command;
  command = "scp " + tm_node->conf_home + tm_conf + " " + user + "@" + tm_node->testbed_ip + ":" + tm_node->conf_home;
  cout << command << endl;
  scp_command = popen (command.c_str (), "r");
  /* close */
  pclose (scp_command);
}

void
network::start_tm ()
{
  FILE *ssh_command;
  string command;
  /*kill the topology manager first*/
  command = "ssh " + user + "@" + tm_node->testbed_ip + " -t \"pkill -9 tm\"";
  cout << command << endl;
  ssh_command = popen (command.c_str (), "r");
  if (ssh_command == NULL) {
    cerr << "Failed to stop Topology Manager at node " << tm_node->label << endl;
  }
  pclose (ssh_command);
  /*now start the TM*/
  command = "ssh " + user + "@" + tm_node->testbed_ip + " -t \"/home/" + user + "/blackadder/TopologyManager/tm " + tm_node->conf_home + "topology.graphml > /dev/null 2>&1 &\"";
  cout << command << endl;
  ssh_command = popen (command.c_str (), "r");
  if (ssh_command == NULL) {
    cerr << "Failed to start Topology Manager at node " << tm_node->label << endl;
  }
  pclose (ssh_command);
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

void
network::create_graph ()
{
  vertex src_v, dst_v;

  map<string, vertex>::iterator vertices_map_iter;

  /* iterate over all nodes in net_graph and add respective vertices in the graph*/
  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
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
      node_ptr dst_n_ptr = nodes[c_ptr->dst_label];
      /* check if destination node is a vertex in the boost graph */
      vertices_map_iter = vertices_map.find (dst_n_ptr->label);
      if (vertices_map_iter == vertices_map.end ()) {
	/* add the node in the boost graph */
	dst_v = add_vertex (dst_n_ptr, net_graph);
	vertices_map.insert (pair<string, vertex> (dst_n_ptr->label, dst_v));
      } else {
	dst_v = (*vertices_map_iter).second;
      }
      /* add the connection in the boost graph */
      add_edge (src_v, dst_v, c_ptr, net_graph);
    }
  }
}

void
network::calculate_forwarding_id (vertex src_v, vertex dst_v, vector<vertex> &predecessor_vector, bitvector &lipsin)
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
network::calculate_forwarding_ids ()
{
  /* a predecessor vector used by BFS */
  vector<vertex> predecessor_vector (boost::num_vertices (net_graph));

  /* get vertex descriptors for these nodes */
  vertex rv_vertex = (*vertices_map.find (rv_node->label)).second;
  vertex tm_vertex = (*vertices_map.find (tm_node->label)).second;

  /* for each node in the network, find the shortest path to rv and tm */
  BOOST_FOREACH(node_map_pair_t node_pair, nodes) {
    /* a shared pointer to each node in the network */
    node_ptr src_n_ptr = node_pair.second;

    /* the respective vertex in the boost graph for each node in the network */
    vertex source_vertex = (*vertices_map.find (src_n_ptr->label)).second;

    /* all weights are 1 so, as boost suggests, I am running a BFS with a predecessor map */
    boost::breadth_first_search (net_graph, source_vertex, boost::visitor (boost::make_bfs_visitor (boost::record_predecessors (&predecessor_vector[0], boost::on_tree_edge ()))));

    /* calculate lipsin identifier to the rv node - use the predecessor map above */
    src_n_ptr->lipsin_rv = bitvector (link_id_len * 8);
    calculate_forwarding_id (source_vertex, rv_vertex, predecessor_vector, src_n_ptr->lipsin_rv);

    /* calculate lipsin identifier to the tm node - use the predecessor map above */
    src_n_ptr->lipsin_tm = bitvector (link_id_len * 8);
    calculate_forwarding_id (source_vertex, tm_vertex, predecessor_vector, src_n_ptr->lipsin_tm);
  }
}

void
network::print_graph ()
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
      click_home = pt.get<string> ("click_home", "/home/" + user + "/click/");
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
    } else if (overlay_mode.compare ("IP") == 0) {
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

