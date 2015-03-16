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
read_topology (network_graph_ptr net_graph_ptr, std::string filename)
{
  boost::dynamic_properties dp;

  boost::filesystem::path topology_path (filename);
  boost::filesystem::ifstream topology_istream;

  if (!boost::filesystem::exists (topology_path)) {
    cout << "Topology file " << topology_path << " does not exist. Aborting..." << endl;
    exit (EXIT_FAILURE);
  }

  read_graphviz (topology_istream, *net_graph_ptr, dp);
}

//Bitvector *tm_graph::calculateFID(string &source, string &destination) {
//    int vertex_id;
//    Bitvector *result = new Bitvector(FID_LEN * 8);
//    igraph_vs_t vs;
//    igraph_vector_ptr_t res;
//    igraph_vector_t to_vector;
//    igraph_vector_t *temp_v;
//    igraph_integer_t eid;
//
//    /*find the vertex id in the reverse index*/
//    int from = (*reverse_node_index.find(source)).second;
//    igraph_vector_init(&to_vector, 1);
//    VECTOR(to_vector)[0] = (*reverse_node_index.find(destination)).second;
//    /*initialize the sequence*/
//    igraph_vs_vector(&vs, &to_vector);
//    /*initialize the vector that contains pointers*/
//    igraph_vector_ptr_init(&res, 1);
//    temp_v = (igraph_vector_t *) VECTOR(res)[0];
//    temp_v = (igraph_vector_t *) malloc(sizeof (igraph_vector_t));
//    VECTOR(res)[0] = temp_v;
//    igraph_vector_init(temp_v, 1);
//    /*run the shortest path algorithm from "from"*/
//#if IGRAPH_V >= IGRAPH_V_0_6
//    igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT);
//#else
//    igraph_get_shortest_paths(&graph, &res, from, vs, IGRAPH_OUT);
//#endif
//    /*check the shortest path to each destination*/
//    temp_v = (igraph_vector_t *) VECTOR(res)[0];
//    //click_chatter("Shortest path from %s to %s", igraph_cattribute_VAS(&graph, "NODEID", from), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[igraph_vector_size(temp_v) - 1]));
//    /*now let's "or" the FIDs for each link in the shortest path*/
//    for (int j = 0; j < igraph_vector_size(temp_v) - 1; j++) {
//#if IGRAPH_V >= IGRAPH_V_0_6
//        igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true, true);
//#else
//        igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
//#endif
//        //click_chatter("node %s -> node %s", igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j]), igraph_cattribute_VAS(&graph, "NODEID", VECTOR(*temp_v)[j + 1]));
//        //click_chatter("link: %s", igraph_cattribute_EAS(&graph, "LID", eid));
//        string LID(igraph_cattribute_EAS(&graph, "LID", eid), FID_LEN * 8);
//        for (int k = 0; k < FID_LEN * 8; k++) {
//            if (LID[k] == '1') {
//                (*result)[ FID_LEN * 8 - k - 1].operator |=(true);
//            }
//        }
//        //click_chatter("FID of the shortest path: %s", result.to_string().c_str());
//    }
//    /*now for all destinations "or" the internal linkID*/
//    vertex_id = (*reverse_node_index.find(destination)).second;
//    string iLID(igraph_cattribute_VAS(&graph, "iLID", vertex_id));
//    //click_chatter("internal link for node %s: %s", igraph_cattribute_VAS(&graph, "NODEID", vertex_id), iLID.c_str());
//    for (int k = 0; k < FID_LEN * 8; k++) {
//        if (iLID[k] == '1') {
//            (*result)[ FID_LEN * 8 - k - 1].operator |=(true);
//        }
//    }
//    igraph_vector_destroy((igraph_vector_t *) VECTOR(res)[0]);
//    igraph_vector_destroy(&to_vector);
//    igraph_vector_ptr_destroy_all(&res);
//    igraph_vs_destroy(&vs);
//    return result;
//}
//
///*main function for rendezvous*/
//void tm_graph::calculateFID(set<string> &publishers, set<string> &subscribers, map<string, Bitvector *> &result) {
//    set<string>::iterator subscribers_it;
//    set<string>::iterator publishers_it;
//    string bestPublisher;
//    Bitvector resultFID(FID_LEN * 8);
//    Bitvector bestFID(FID_LEN * 8);
//    unsigned int numberOfHops = 0;
//    /*first add all publishers to the hashtable with NULL FID*/
//    for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
//        string publ = *publishers_it;
//        result.insert(pair<string, Bitvector *>(publ, NULL));
//    }
//    for (subscribers_it = subscribers.begin(); subscribers_it != subscribers.end(); subscribers_it++) {
//        /*for all subscribers calculate the number of hops from all publishers (not very optimized...don't you think?)*/
//        unsigned int minimumNumberOfHops = UINT_MAX;
//        for (publishers_it = publishers.begin(); publishers_it != publishers.end(); publishers_it++) {
//            resultFID.clear();
//            string str1 = (*publishers_it);
//            string str2 = (*subscribers_it);
//            calculateFID(str1, str2, resultFID, numberOfHops);
//            if (minimumNumberOfHops > numberOfHops) {
//                minimumNumberOfHops = numberOfHops;
//                bestPublisher = *publishers_it;
//                bestFID = resultFID;
//            }
//        }
//        //cout << "best publisher " << bestPublisher << " for subscriber " << (*subscribers_it) << " -- number of hops " << minimumNumberOfHops - 1 << endl;
//        if ((*result.find(bestPublisher)).second == NULL) {
//            /*add the publisher to the result*/
//            //cout << "FID1: " << bestFID.to_string() << endl;
//            result[bestPublisher] = new Bitvector(bestFID);
//        } else {
//            //cout << "/*update the FID for the publisher*/" << endl;
//            Bitvector *existingFID = (*result.find(bestPublisher)).second;
//            /*or the result FID*/
//            *existingFID = *existingFID | bestFID;
//        }
//    }
//}
//
//void tm_graph::calculateFID(string &source, string &destination, Bitvector &resultFID, unsigned int &numberOfHops) {
//    igraph_vs_t vs;
//    igraph_vector_ptr_t res;
//    igraph_vector_t to_vector;
//    igraph_vector_t *temp_v;
//    igraph_integer_t eid;
//
//    /*find the vertex id in the reverse index*/
//    int from = (*reverse_node_index.find(source)).second;
//    igraph_vector_init(&to_vector, 1);
//    VECTOR(to_vector)[0] = (*reverse_node_index.find(destination)).second;
//    /*initialize the sequence*/
//    igraph_vs_vector(&vs, &to_vector);
//    /*initialize the vector that contains pointers*/
//    igraph_vector_ptr_init(&res, 1);
//    temp_v = (igraph_vector_t *) VECTOR(res)[0];
//    temp_v = (igraph_vector_t *) malloc(sizeof (igraph_vector_t));
//    VECTOR(res)[0] = temp_v;
//    igraph_vector_init(temp_v, 1);
//    /*run the shortest path algorithm from "from"*/
//#if IGRAPH_V >= IGRAPH_V_0_6
//    igraph_get_shortest_paths(&graph, &res, NULL, from, vs, IGRAPH_OUT);
//#else
//    igraph_get_shortest_paths(&graph, &res, from, vs, IGRAPH_OUT);
//#endif
//    /*check the shortest path to each destination*/
//    temp_v = (igraph_vector_t *) VECTOR(res)[0];
//
//    /*now let's "or" the FIDs for each link in the shortest path*/
//    for (int j = 0; j < igraph_vector_size(temp_v) - 1; j++) {
//#if IGRAPH_V >= IGRAPH_V_0_6
//        igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true, true);
//#else
//        igraph_get_eid(&graph, &eid, VECTOR(*temp_v)[j], VECTOR(*temp_v)[j + 1], true);
//#endif
//        Bitvector *lid = (*edge_LID.find(eid)).second;
//        (resultFID) = (resultFID) | (*lid);
//    }
//    numberOfHops = igraph_vector_size(temp_v);
//
//    /*now for the destination "or" the internal linkID*/
//    Bitvector *ilid = (*nodeID_iLID.find(destination)).second;
//    (resultFID) = (resultFID) | (*ilid);
//    //cout << "FID of the shortest path: " << resultFID.to_string() << endl;
//    igraph_vector_destroy((igraph_vector_t *) VECTOR(res)[0]);
//    igraph_vector_destroy(&to_vector);
//    igraph_vector_ptr_destroy_all(&res);
//    igraph_vs_destroy(&vs);
//}
