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

#include "graph.h"

using namespace std;

/* maps node labels to vertex descriptors in the boost graph */
map<std::string, vertex> vertices_map;

/* Property Writer Functors for writing Graphviz graphs (oh dear...) */

class graph_property_writer
{
public:
  graph_property_writer (network_graph_ptr net_graph_ptr) :
      net_graph_ptr (net_graph_ptr)
  {
  }
  void
  operator() (std::ostream &out) const
  {
    out << "link_id_len=\"" << (*net_graph_ptr)[boost::graph_bundle]->link_id_len << "\";" << endl;
    out << "tm_node=\"" << (*net_graph_ptr)[boost::graph_bundle]->tm_node->label << "\";" << endl;
    out << "rv_node=\"" << (*net_graph_ptr)[boost::graph_bundle]->rv_node->label << "\";" << endl;
    out << "tm_mode=\"" << (*net_graph_ptr)[boost::graph_bundle]->tm_node->running_mode << "\";" << endl;
  }
private:
  network_graph_ptr net_graph_ptr;
};

class vertex_property_writer
{
public:
  vertex_property_writer (network_graph_ptr net_graph_ptr) :
      net_graph_ptr (net_graph_ptr)
  {
  }
  template<class Vertex>
    void
    operator() (std::ostream &out, const Vertex& v) const
    {
      out << "[label=\"" << (*net_graph_ptr)[v]->label << ",internal_link_id=\"" << (*net_graph_ptr)[v]->internal_link_id.to_string () << "\"]";
    }
private:
  network_graph_ptr net_graph_ptr;
};

class edge_property_writer
{
public:
  edge_property_writer (network_graph_ptr net_graph_ptr) :
      net_graph_ptr (net_graph_ptr)
  {
  }
  template<class Edge>
    void
    operator() (std::ostream &out, const Edge& e) const
    {
      out << "[link_id=\"" << (*net_graph_ptr)[e]->link_id.to_string () << "\"]";
    }
private:
  network_graph_ptr net_graph_ptr;
};

void
create_graph (network_graph_ptr net_graph_ptr, network_ptr net_ptr)
{
  typedef std::pair<std::string, connection_ptr> connection_map_pair_t;
  typedef std::pair<std::string, node_ptr> node_map_pair_t;

  vertex src_v, dst_v;

  map<string, vertex>::iterator vertices_map_iter;

  /* iterate over all nodes in net_graph and add respective vertices in the graph*/
  BOOST_FOREACH(node_map_pair_t node_pair, net_ptr->nodes) {
    node_ptr src_n_ptr = node_pair.second;
    /* check if this node is already added in the network graph */
    vertices_map_iter = vertices_map.find (src_n_ptr->label);
    if (vertices_map_iter == vertices_map.end ()) {
      /* add the node in the boost graph */
      src_v = add_vertex (src_n_ptr, *net_graph_ptr);
      vertices_map.insert (pair<string, vertex> (src_n_ptr->label, src_v));
    } else {
      src_v = (*vertices_map_iter).second;
    }
    /* iterate over all connections in net_graph and add respective edges in the graph */
    BOOST_FOREACH(connection_map_pair_t connection_pair, src_n_ptr->connections) {
      connection_ptr c_ptr = connection_pair.second;
      node_ptr dst_n_ptr = net_ptr->nodes[c_ptr->dst_label];
      /* check if destination node is a vertex in the boost graph */
      vertices_map_iter = vertices_map.find (dst_n_ptr->label);
      if (vertices_map_iter == vertices_map.end ()) {
	/* add the node in the boost graph */
	dst_v = add_vertex (dst_n_ptr, *net_graph_ptr);
	vertices_map.insert (pair<string, vertex> (dst_n_ptr->label, dst_v));
      } else {
	dst_v = (*vertices_map_iter).second;
      }
      /* add the connection in the boost graph */
      add_edge (src_v, dst_v, c_ptr, *net_graph_ptr);
    }
  }
}

void
assign_link_ids (network_graph_ptr net_graph_ptr)
{
  map<string, bitvector> link_identifiers;
  map<string, bitvector>::iterator link_identifiers_iterator;

  int lid_counter = 0;
  int lid_len = (*net_graph_ptr)[boost::graph_bundle]->link_id_len;
  srand (0);

  /* how many link identifiers should I calculate? */
  int total_ids;

  /* internal link identifiers */
  total_ids = num_vertices (*net_graph_ptr) + num_edges (*net_graph_ptr);

  for (int i = 0; i < total_ids; i++) {
    u_int32_t bit_position;
    u_int32_t number_of_bits = (i / (lid_len * 8)) + 1;
    bitvector link_identifier;
    do {
      link_identifier = bitvector (lid_len * 8);
      for (int i = 0; i < number_of_bits; i++) {
	/* assign a bit in a random position */
	bit_position = rand () % (lid_len * 8);
	link_identifier[bit_position] = true;
      }
      /* eliminate duplicate link identifiers */
    } while (link_identifiers.find (link_identifier.to_string ()) != link_identifiers.end ());

    link_identifiers.insert (pair<string, bitvector> (link_identifier.to_string (), link_identifier));

  }

  link_identifiers_iterator = link_identifiers.begin ();

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {
    (*net_graph_ptr)[v]->internal_link_id = (*link_identifiers_iterator).second;
    link_identifiers_iterator++;
  }

  /* iterate over all edges in the boost graph */
  BOOST_FOREACH(edge e, edges(*net_graph_ptr)) {
    (*net_graph_ptr)[e]->link_id = (*link_identifiers_iterator).second;
    link_identifiers_iterator++;
  }
}

void
calculate_forwarding_id (network_graph_ptr net_graph_ptr, vertex src_v, vertex dst_v, vector<vertex> &predecessor_vector, bitvector &lipsin)
{
  vertex predeccesor;
  node_ptr n;

  /* source node is the same as destination */
  if (dst_v == src_v) {
    /* XOR lipsin with dst_v == src_v internal_link_id and return */
    lipsin ^= (*net_graph_ptr)[dst_v]->internal_link_id;
    return;
  }

  while (true) {
    /* XOR lipsin with dst_v internal_link_id */
    n = (*net_graph_ptr)[dst_v];
    lipsin ^= n->internal_link_id;

    /* find the predeccesor node */
    predeccesor = predecessor_vector[dst_v];
    pair<edge, bool> edge_pair = boost::edge (predeccesor, dst_v, *net_graph_ptr);
    if (edge_pair.second == false) {
      /* this should never happen - an edge must always exist */
      cout << "Fatal: No edge between " << (*net_graph_ptr)[predeccesor]->label << " and " << (*net_graph_ptr)[dst_v]->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* XOR with edge's link_id */
    lipsin ^= (*net_graph_ptr)[edge_pair.first]->link_id;

    /* done */
    if (predeccesor == src_v) {
      break;
    }

    /* move on to the next iteration */
    dst_v = predeccesor;
  }
}

void
calculate_forwarding_ids (network_graph_ptr net_graph_ptr)
{
  int lid_len = (*net_graph_ptr)[boost::graph_bundle]->link_id_len;

  /* a predecessor vector used by BFS */
  vector<vertex> predecessor_vector (boost::num_vertices (*net_graph_ptr));

  /* get the vertex descriptor for the target node from the vertices_map (global variable) */
  vertex rv_v = (*vertices_map.find ((*net_graph_ptr)[boost::graph_bundle]->rv_node->label)).second;
  vertex tm_v = (*vertices_map.find ((*net_graph_ptr)[boost::graph_bundle]->tm_node->label)).second;

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {
    /* all weights are 1 so, as boost suggests, I am running a BFS with a predecessor map */
    breadth_first_search (*net_graph_ptr, v, boost::visitor (boost::make_bfs_visitor (boost::record_predecessors (&predecessor_vector[0], boost::on_tree_edge ()))));

    /* calculate lipsin identifier to the target node - use the predecessor map above */
    (*net_graph_ptr)[v]->lipsin_rv = bitvector (lid_len * 8);
    calculate_forwarding_id (net_graph_ptr, v, rv_v, predecessor_vector, (*net_graph_ptr)[v]->lipsin_rv);

    (*net_graph_ptr)[v]->lipsin_tm = bitvector (lid_len * 8);
    calculate_forwarding_id (net_graph_ptr, v, tm_v, predecessor_vector, (*net_graph_ptr)[v]->lipsin_tm);
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
discover_mac_addresses (network_graph_ptr net_graph_ptr)
{
  typedef pair<string, node_ptr> node_map_pair_t;
  typedef pair<string, connection_ptr> connection_map_pair_t;

  map<string, string> mac_addresses;

  string line;
  FILE *fp_command;
  string command;
  char response[1035];

  string hwaddr_label;
  int hwaddr_offset;

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {

    /* iterate over all outgoing edges in the boost graph */
    BOOST_FOREACH(edge e, out_edges(v, *net_graph_ptr)) {
      /* get the destination vertex in the boost graph */
      vertex dst_v = target (e, *net_graph_ptr);

      string src_label = (*net_graph_ptr)[e]->src_label;
      string src_if = (*net_graph_ptr)[e]->src_if;

      string dst_label = (*net_graph_ptr)[e]->dst_label;
      string dst_if = (*net_graph_ptr)[e]->dst_if;

      if ((*net_graph_ptr)[e]->overlay_mode.compare ("Ethernet") == 0) {
	if (mac_addresses.find (src_label + src_if) == mac_addresses.end ()) {
	  /* get source mac address */
	  if ((*net_graph_ptr)[e]->src_mac.compare ("unspecified") == 0) {
	    getHwaddrLabel ((*net_graph_ptr)[v]->operating_system, hwaddr_label, hwaddr_offset);
	    if ((*net_graph_ptr)[v]->sudo) {
	      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"sudo ifconfig " + src_if + " | grep " + hwaddr_label + "\"";
	    } else {
	      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"ifconfig " + src_if + " | grep " + hwaddr_label + "\"";
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
	    (*net_graph_ptr)[e]->src_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << (*net_graph_ptr)[e]->src_mac << endl;
	    pclose (fp_command);
	  } else {
	    cout << "Interface " << src_if << " pre-configured with " << (*net_graph_ptr)[e]->src_mac << " for connection " << src_label << " - " << dst_label << endl;
	  }
	  mac_addresses[src_label + src_if] = (*net_graph_ptr)[e]->src_mac;
	} else {
	  (*net_graph_ptr)[e]->src_mac = mac_addresses[src_label + src_if];
	}

	if (mac_addresses.find (dst_label + dst_if) == mac_addresses.end ()) {
	  /*get destination mac address*/
	  if ((*net_graph_ptr)[e]->dst_mac.compare ("unspecified") == 0) {
	    getHwaddrLabel ((*net_graph_ptr)[dst_v]->operating_system, hwaddr_label, hwaddr_offset);
	    if ((*net_graph_ptr)[dst_v]->sudo) {
	      command = "ssh " + (*net_graph_ptr)[dst_v]->user + "@" + (*net_graph_ptr)[dst_v]->testbed_ip + " -t \"sudo ifconfig " + dst_if + " | grep " + hwaddr_label + "\"";
	    } else {
	      command = "ssh " + (*net_graph_ptr)[dst_v]->user + "@" + (*net_graph_ptr)[dst_v]->testbed_ip + " -t \"ifconfig " + dst_if + " | grep " + hwaddr_label + "\"";
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
	    (*net_graph_ptr)[e]->dst_mac = line.substr (line.length () - hwaddr_offset, 17);
	    cout << (*net_graph_ptr)[e]->dst_mac << endl;
	    pclose (fp_command);
	  } else {
	    cout << "Interface " << dst_if << " pre-configured with " << (*net_graph_ptr)[e]->dst_mac << " for connection " << src_label << " - " << dst_label << endl;
	  }
	  mac_addresses[dst_label + dst_if] = (*net_graph_ptr)[e]->dst_mac;
	} else {
	  /* learned this MAC address before */
	  (*net_graph_ptr)[e]->dst_mac = mac_addresses[dst_label + dst_if];
	}
      } else {
	cout << "Connection " << src_label << " - " << dst_label << " is over IP. Nothing to discover here..." << endl;
      }
    }
  }
}

// TODO: ns-3 support
void
write_click_conf (network_graph_ptr net_graph_ptr, string &output_folder)
{
  path output_folder_path (output_folder);

  if (!boost::filesystem::exists (output_folder_path)) {
    cout << "creating directory " << output_folder_path << " to store configuration files before deploying blackadder" << endl;
    boost::filesystem::create_directory (output_folder_path);
  }

  if (!boost::filesystem::is_directory (output_folder_path)) {
    cerr << output_folder_path << " is a regular file. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  /* all Click configuration files will be store locally before they are sent to remote nodes */

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {

    map<string, int> unique_ifaces;
    map<string, int> unique_srcips;
    map<string, int>::iterator unique_ifaces_iterator;
    set<string> link_local_entries;

    int unique_iface_index = 1;
    int unique_ip_index = 1;

    path click_conf_file_path (output_folder_path / ((*net_graph_ptr)[v]->label + ".conf"));

    cout << "writing Click configuration for blackadder node " << (*net_graph_ptr)[v]->label << " at " << click_conf_file_path << endl;

    if (boost::filesystem::exists (click_conf_file_path)) {
      boost::filesystem::remove (click_conf_file_path);
    }

    boost::filesystem::ofstream click_conf;

    click_conf.open (click_conf_file_path);

    /* iterate over all outgoing edges in the boost graph */
    BOOST_FOREACH(edge e, out_edges(v, *net_graph_ptr)) {
      if ((*net_graph_ptr)[e]->overlay_mode.compare ("Ethernet") == 0) {
	if (unique_ifaces.find ((*net_graph_ptr)[e]->src_if) == unique_ifaces.end ()) {
	  unique_ifaces.insert (std::pair<string, int> ((*net_graph_ptr)[e]->src_if, unique_iface_index));
	  unique_iface_index++;
	}
      } else if ((*net_graph_ptr)[e]->overlay_mode.compare ("IP") == 0) {
	if (unique_srcips.find ((*net_graph_ptr)[e]->src_ip) == unique_srcips.end ()) {
	  unique_srcips.insert (std::pair<string, int> ((*net_graph_ptr)[e]->src_ip, unique_ip_index));
	  unique_ip_index++;
	}
      } else {
	cout << "Unknown overlay mode. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
    }

    click_conf << "require(blackadder);" << endl << endl;
    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      if ((*net_graph_ptr)[v]->running_mode.compare ("kernel") == 0) {
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
    click_conf << "dispatcher::Dispatcher(NODEID " << (*net_graph_ptr)[v]->label << ",DEFAULTRV " << (*net_graph_ptr)[v]->lipsin_rv.to_string () << ");" << endl << endl;
    click_conf << "rv::RV(NODEID " << (*net_graph_ptr)[v]->label << ",TMFID " << (*net_graph_ptr)[v]->lipsin_tm.to_string () << ");" << endl << endl;
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

    /* iterate over all outgoing edges in the boost graph */
    BOOST_FOREACH(edge e, out_edges(v, *net_graph_ptr)) {
      /*1 is the link-local forwarding strategy*/
      if ((*net_graph_ptr)[e]->overlay_mode.compare ("Ethernet") == 0) {
	if (link_local_entries.find ((*net_graph_ptr)[e]->src_mac + (*net_graph_ptr)[e]->dst_mac) == link_local_entries.end ()) {
	  link_local_entries.insert ((*net_graph_ptr)[e]->src_mac + (*net_graph_ptr)[e]->dst_mac);
	}
      } else if ((*net_graph_ptr)[e]->overlay_mode.compare ("IP") == 0) {
	if (link_local_entries.find ((*net_graph_ptr)[e]->src_ip + (*net_graph_ptr)[e]->dst_ip) == link_local_entries.end ()) {
	  link_local_entries.insert ((*net_graph_ptr)[e]->src_ip + (*net_graph_ptr)[e]->dst_ip);
	}
      }
    }
    click_conf << (*net_graph_ptr)[v]->connections.size () + link_local_entries.size () + 1 << "," << endl;
    /*Add the Internal Link for Lipsin based strategy*/
    click_conf << "2,0,INTERNAL," << (*net_graph_ptr)[v]->internal_link_id.to_string () << "," << endl;

    /* iterate over all outgoing edges in the boost graph */
    BOOST_FOREACH(edge e, out_edges(v, *net_graph_ptr)) {
      if ((*net_graph_ptr)[e]->overlay_mode.compare ("Ethernet") == 0) {
	/* 2 is the LIPSIN based forwarding strategy */
	click_conf << "2," << (*unique_ifaces.find ((*net_graph_ptr)[e]->src_if)).second << ",MAC," << (*net_graph_ptr)[e]->src_mac << "," << (*net_graph_ptr)[e]->dst_mac << ","
	    << (*net_graph_ptr)[e]->link_id.to_string () << "," << endl;
      } else if ((*net_graph_ptr)[e]->overlay_mode.compare ("IP") == 0) {
	click_conf << "2," << unique_ifaces.size () + 1 << ",IP," << (*net_graph_ptr)[e]->src_ip << "," << (*net_graph_ptr)[e]->dst_ip << "," << (*net_graph_ptr)[e]->link_id.to_string () << ","
	    << endl;
      } else {
	cout << "Unknown overlay mode. Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
    }

    /*now add the link strategy entries*/
    link_local_entries.clear ();
    /* iterate over all outgoing edges in the boost graph */
    BOOST_FOREACH(edge e, out_edges(v, *net_graph_ptr)) {
      /* 1 is the link-local forwarding strategy */
      if ((*net_graph_ptr)[e]->overlay_mode.compare ("Ethernet") == 0) {
	if (link_local_entries.find ((*net_graph_ptr)[e]->src_mac + (*net_graph_ptr)[e]->dst_mac) == link_local_entries.end ()) {
	  link_local_entries.insert ((*net_graph_ptr)[e]->src_mac + (*net_graph_ptr)[e]->dst_mac);
	  click_conf << "1," << (*unique_ifaces.find ((*net_graph_ptr)[e]->src_if)).second << ",MAC," << (*net_graph_ptr)[e]->src_mac << "," << (*net_graph_ptr)[e]->dst_mac << "," << endl;
	}
      } else if ((*net_graph_ptr)[e]->overlay_mode.compare ("IP") == 0) {
	if (link_local_entries.find ((*net_graph_ptr)[e]->src_ip + (*net_graph_ptr)[e]->dst_ip) == link_local_entries.end ()) {
	  link_local_entries.insert ((*net_graph_ptr)[e]->src_ip + (*net_graph_ptr)[e]->dst_ip);
	  click_conf << "1," << unique_ifaces.size () + 1 << ",IP," << (*net_graph_ptr)[e]->src_ip << "," << (*net_graph_ptr)[e]->dst_ip << "," << endl;
	} else {
	  cout << "Unknown overlay mode. Aborting..." << endl;
	  exit (EXIT_FAILURE);
	}
      }
    }

    click_conf << ");" << endl << endl;

    /*add network devices*/
    for (unique_ifaces_iterator = unique_ifaces.begin (); unique_ifaces_iterator != unique_ifaces.end (); unique_ifaces_iterator++) {
      if ((*net_graph_ptr)[v]->running_mode.compare ("user") == 0) {
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "::FromDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl << "todev" << (*unique_ifaces_iterator).second
	    << "::ToDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl;
      } else {
	click_conf << "fromdev" << (*unique_ifaces_iterator).second << "::FromDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl << "todev" << (*unique_ifaces_iterator).second
	    << "::ToDevice(" << (*unique_ifaces_iterator).first << ",BURST 8);" << endl;
	click_conf << "tohost::ToHost();" << endl;
      }
    }
    if (unique_srcips.size () > 0) {
      if ((*net_graph_ptr)[v]->running_mode.compare ("user") == 0) {
	click_conf << "rawsocket" << "::RawSocket(UDP, 55555);" << endl;
      } else {
	cerr << "Something is wrong...I should not build click config using raw sockets for node " << (*net_graph_ptr)[v]->label << "that will run in kernel space. Aborting..." << endl;
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
      if ((*net_graph_ptr)[v]->running_mode.compare ("kernel") == 0) {
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
write_tm_conf (network_graph_ptr net_graph_ptr, string &output_folder)
{

  path output_folder_path (output_folder);
  boost::filesystem::ofstream tm_conf;

  if (!boost::filesystem::exists (output_folder_path)) {
    cout << "creating directory " << output_folder_path << " to store configuration files before deploying blackadder" << endl;
    boost::filesystem::create_directory (output_folder_path);
  }

  if (!boost::filesystem::is_directory (output_folder_path)) {
    cerr << output_folder_path << " is a regular file. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  path tm_conf_file_path (output_folder_path / ("topology.graph"));

  cout << "writing TM configuration for blackadder node " << (*net_graph_ptr)[boost::graph_bundle]->tm_node->label << " at " << tm_conf_file_path << endl;

  if (boost::filesystem::exists (tm_conf_file_path)) {
    boost::filesystem::remove (tm_conf_file_path);
  }

  tm_conf.open (tm_conf_file_path);

  boost::write_graphviz (tm_conf, *net_graph_ptr, vertex_property_writer (net_graph_ptr), edge_property_writer (net_graph_ptr), graph_property_writer (net_graph_ptr));

  tm_conf.close ();
}

void
scp_click_conf (network_graph_ptr net_graph_ptr, string &output_folder)
{
  FILE *scp_command;
  string command;

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {
    command = "scp " + output_folder + "/" + (*net_graph_ptr)[v]->label + ".conf" + " " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + ":" + (*net_graph_ptr)[v]->conf_home;

    cout << command << endl;

    scp_command = popen (command.c_str (), "r");

    if (scp_command == NULL) {
      cerr << "Failed to scp click configuration file to node " << (*net_graph_ptr)[v]->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    /* close */
    pclose (scp_command);
  }
}

void
scp_tm_conf (network_graph_ptr net_graph_ptr, string &output_folder)
{
  FILE *scp_command;
  string command;
  command = "scp " + output_folder + "/topology.graph" + " " + (*net_graph_ptr)[boost::graph_bundle]->tm_node->user + "@" + (*net_graph_ptr)[boost::graph_bundle]->tm_node->testbed_ip + ":"
      + (*net_graph_ptr)[boost::graph_bundle]->tm_node->conf_home;
  cout << command << endl;
  scp_command = popen (command.c_str (), "r");
  /* close */
  pclose (scp_command);
}

void
start_click (network_graph_ptr net_graph_ptr)
{
  FILE *ssh_command;
  string command;

  /* iterate over all vertices in the boost graph */
  BOOST_FOREACH(vertex v, vertices(*net_graph_ptr)) {

    /*kill click first both from kernel and user space*/
    if ((*net_graph_ptr)[v]->sudo) {
      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"sudo pkill -9 click\"";
    } else {
      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"pkill -9 click\"";
    }
    cout << command << endl;
    ssh_command = popen (command.c_str (), "r");
    if (ssh_command == NULL) {
      cerr << "Failed to stop click at node " << (*net_graph_ptr)[v]->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    pclose (ssh_command);
    if ((*net_graph_ptr)[v]->sudo) {
      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"sudo " + (*net_graph_ptr)[v]->click_home + "/sbin/click-uninstall\"";
    } else {
      command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " -t \"" + (*net_graph_ptr)[v]->click_home + "/sbin/click-uninstall \"";
    }
    cout << command << endl;
    ssh_command = popen (command.c_str (), "r");
    if (ssh_command == NULL) {
      cerr << "Failed to stop click at node " << (*net_graph_ptr)[v]->label << ". Aborting..." << endl;
      exit (EXIT_FAILURE);
    }
    pclose (ssh_command);
    /*now start click*/
    if ((*net_graph_ptr)[v]->running_mode.compare ("user") == 0) {
      if ((*net_graph_ptr)[v]->sudo) {
	command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " \"sudo " + (*net_graph_ptr)[v]->click_home + "/bin/click " + (*net_graph_ptr)[v]->conf_home
	    + (*net_graph_ptr)[v]->label + ".conf > /dev/null 2>&1 &\"";
      } else {
	command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " \"" + (*net_graph_ptr)[v]->click_home + "/bin/click " + (*net_graph_ptr)[v]->conf_home
	    + (*net_graph_ptr)[v]->label + ".conf > /dev/null 2>&1 &\"";
      }
      cout << command << endl;
      ssh_command = popen (command.c_str (), "r");
      if (ssh_command == NULL) {
	cerr << "Failed to start click at node " << (*net_graph_ptr)[v]->label << ". Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
      pclose (ssh_command);
    } else {
      if ((*net_graph_ptr)[v]->sudo) {
	command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " \"sudo " + (*net_graph_ptr)[v]->click_home + "/sbin/click-install " + (*net_graph_ptr)[v]->conf_home
	    + (*net_graph_ptr)[v]->label + ".conf > /dev/null 2>&1 &\"";
      } else {
	command = "ssh " + (*net_graph_ptr)[v]->user + "@" + (*net_graph_ptr)[v]->testbed_ip + " \"" + (*net_graph_ptr)[v]->click_home + "/sbin/click-install " + (*net_graph_ptr)[v]->conf_home
	    + (*net_graph_ptr)[v]->label + ".conf > /dev/null 2>&1 &\"";
      }
      cout << command << endl;
      ssh_command = popen (command.c_str (), "r");
      if (ssh_command == NULL) {
	cerr << "Failed to start click at node " << (*net_graph_ptr)[v]->label << ". Aborting..." << endl;
	exit (EXIT_FAILURE);
      }
      pclose (ssh_command);
    }
  }
}

void
start_tm (network_graph_ptr net_graph_ptr)
{
  FILE *ssh_command;
  string command;
  /*kill the topology manager first*/
  command = "ssh " + (*net_graph_ptr)[boost::graph_bundle]->tm_node->user + "@" + (*net_graph_ptr)[boost::graph_bundle]->tm_node->testbed_ip + " -t \"pkill -9 tm\"";
  cout << command << endl;
  ssh_command = popen (command.c_str (), "r");
  if (ssh_command == NULL) {
    cerr << "Failed to stop Topology Manager at node " << (*net_graph_ptr)[boost::graph_bundle]->tm_node->label << endl;
  }
  pclose (ssh_command);
  /*now start the TM*/
  command = "ssh " + (*net_graph_ptr)[boost::graph_bundle]->tm_node->user + "@" + (*net_graph_ptr)[boost::graph_bundle]->tm_node->testbed_ip + " -t \"/home/"
      + (*net_graph_ptr)[boost::graph_bundle]->tm_node->user + "/blackadder/TopologyManager/tm " + (*net_graph_ptr)[boost::graph_bundle]->tm_node->conf_home + "topology.graph > /dev/null 2>&1 &\"";
  cout << command << endl;
  ssh_command = popen (command.c_str (), "r");
  if (ssh_command == NULL) {
    cerr << "Failed to start Topology Manager at node " << (*net_graph_ptr)[boost::graph_bundle]->tm_node->label << endl;
  }
  pclose (ssh_command);
}

void
print_graph (network_graph_ptr net_graph_ptr)
{
  boost::write_graphviz (cout, *net_graph_ptr, vertex_property_writer (net_graph_ptr), edge_property_writer (net_graph_ptr), graph_property_writer (net_graph_ptr));
}
