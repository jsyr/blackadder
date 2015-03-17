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

#include "tm_graph.h"

using namespace std;

void
parse_configuration (boost::property_tree::ptree &pt, const string &filename)
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

  try {
    // Load the .xml file into the property tree. If reading fails
    // (cannot open file, parse error), an exception is thrown.
    read_xml (conf_path_stream, pt);
  } catch (boost::property_tree::xml_parser_error &err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  }
}

void
load_network (network_ptr net_ptr, const string &filename)
{
  // Create an empty property tree object
  using boost::property_tree::ptree;
  ptree pt;

  /* parse configuration file */
  parse_configuration (pt, filename);

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
      /* all connections MUST be unidirectional (that;s the case when the inout file is auto-generated by the deployment tool) */
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
    }
  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    /* this is allowed to support one node deployments for debugging */
    cerr << "No connections are defined " << endl;
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
    if (n_ptr->label.length () != NODEID_LEN) {
      cerr << "Label " << n_ptr->label << " is not " << NODEID_LEN << " bytes long. Aborting..." << endl;
      exit (EXIT_FAILURE);
    }

    /* optional - with default values */
    n_ptr->is_rv = pt.get<bool> ("is_rv", false);
    n_ptr->is_tm = pt.get<bool> ("is_tm", false);

//    n_ptr->internal_link_id = bitvector (pt.get<string> ("internal_link_id"));

    bitvector test("00");
    cout << test.to_string() << endl;

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

//    c_ptr->link_id = bitvector (pt.get<string> ("link_id"));

  } catch (boost::property_tree::ptree_bad_data& err) {
    cerr << err.what () << endl;
    exit (EXIT_FAILURE);
  } catch (boost::property_tree::ptree_bad_path& err) {
    cerr << "missing mandatory connection parameter - " << err.what () << endl;
    exit (EXIT_FAILURE);
  }
}
