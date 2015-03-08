/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
 * PlanetLab Deployment support By Dimitris Syrivelis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#include <vector>

#include "graph_representation.hpp"


bitvector
GraphRepresentation::calculateFID (string &source, string &destination)
{
  int vertex_id;
  bitvector result (dm->fid_len * 8);
  igraph_vs_t vs;
  igraph_vector_ptr_t res;
  igraph_vector_t to_vector;
  igraph_vector_t *temp_v;
  igraph_integer_t eid;
  const char *Ntype;
  /*find the vertex id in the reverse index*/
  map<string, int>::iterator index_it = reverse_node_index.find (source);
  if (index_it == reverse_node_index.end ()) {
    cerr << "Warning: " << source << " not found in reverse node index map (source)" << endl;
    return result; /* XXX */
  }
  int from = index_it->second;

  cout << "Creating RVFIDs and TMFIDs for single layer topology..." << endl;

  index_it = reverse_node_index.find (destination);
  if (index_it == reverse_node_index.end ()) {
    cerr << "Warning: " << destination << " not found in reverse node index map (destination)" << endl;
    return result; /* XXX */
  }
  igraph_vector_init (&to_vector, 1);
  VECTOR(to_vector)[0] = index_it->second;  // find vertex id of the destination node label
  /*initialize the sequence*/
  igraph_vs_vector (&vs, &to_vector);
  /*initialize the vector that contains pointers*/
  igraph_vector_ptr_init (&res, 1);
  temp_v = (igraph_vector_t *) VECTOR(res)[0];
  temp_v = (igraph_vector_t *) malloc (sizeof(igraph_vector_t));
  VECTOR(res)[0] = temp_v;
  igraph_vector_init (temp_v, 1);
  /*run the shortest path algorithm from "from"*/
#if IGRAPH_V >= IGRAPH_V_0_6
  igraph_get_shortest_paths(&igraph, &res, NULL, from, vs, IGRAPH_OUT);
#else
  igraph_get_shortest_paths (&igraph, &res, from, vs, IGRAPH_OUT);
#endif
  /*check the shortest path to each destination*/
  temp_v = (igraph_vector_t *) VECTOR(res)[0];
  //click_chatter("Shortest path from %s to %s", igraph_cattribute_VAS(&graph, "NODEID", from), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[igraph_vector_size(temp_v) - 1]));

  /*now let's "or" the FIDs for each link in the shortest path*/
  for (int j = 0; j < igraph_vector_size (temp_v) - 1; j++) {
#if IGRAPH_V >= IGRAPH_V_0_6
    igraph_get_eid(&igraph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true, true);
#else
    igraph_get_eid (&igraph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
#endif
    //click_chatter("node %s -> node %s", igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j]), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j + 1]));
    //click_chatter("link: %s", igraph_cattribute_EAS(&graph, "LID", eid));
    string LID (igraph_cattribute_EAS (&igraph, "LID", eid), dm->fid_len * 8);
    for (int k = 0; k < dm->fid_len * 8; k++) {
      if (LID[k] == '1') {
	(result)[dm->fid_len * 8 - k - 1].operator |= (true);
      }
    }
    //click_chatter("FID of the shortest path: %s", result.to_string().c_str());
  }
  /*now for all destinations "or" the internal linkID*/
  vertex_id = (*reverse_node_index.find (destination)).second;
  string iLID (igraph_cattribute_VAS (&igraph, "iLID", vertex_id));
  //click_chatter("internal link for node %s: %s", igraph_cattribute_VAS(&graph, "NODEID", vertex_id), iLID.c_str());
  for (int k = 0; k < dm->fid_len * 8; k++) {
    if (iLID[k] == '1') {
      (result)[dm->fid_len * 8 - k - 1].operator |= (true);
    }
  }
  igraph_vector_destroy ((igraph_vector_t *) VECTOR(res)[0]);
  igraph_vector_destroy (&to_vector);
  igraph_vector_ptr_destroy_all (&res);
  igraph_vs_destroy (&vs);
  return result;
}

void
GraphRepresentation::calculateRVFIDs ()
{
  string RVLabel = dm->RV_node->label;
  for (size_t i = 0; i < dm->network_nodes.size (); i++) {
    network_node *nn = dm->network_nodes[i];
    nn->fid_to_rv = calculateFID (nn->label, RVLabel);
  }
}

void
GraphRepresentation::calculateTMFIDs ()
{
  string TMLabel = dm->TM_node->label;
  for (size_t i = 0; i < dm->network_nodes.size (); i++) {
    network_node *nn = dm->network_nodes[i];
    nn->fid_to_tm = calculateFID (nn->label, TMLabel);
  }
}
