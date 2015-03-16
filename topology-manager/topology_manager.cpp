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

#include <boost/program_options.hpp>

#include <signal.h>
#include <arpa/inet.h>
#include <set>
#include <blackadder.hpp>

#include "tm_graph.h"

using namespace std;

//Blackadder *ba = NULL;
//tm_graph *tm_igraph = NULL;
//pthread_t _event_listener, *event_listener = NULL;
//sig_atomic_t listening = 1;
//
//string req_id = string(PURSUIT_ID_LEN * 2 - 1, 'F') + "E"; // "FF..FFFFFFFFFFFFFE"
//string req_prefix_id = string();
//string req_bin_id = hex_to_chararray(req_id);
//string req_bin_prefix_id = hex_to_chararray(req_prefix_id);
//
//string resp_id = string();
//string resp_prefix_id = string(PURSUIT_ID_LEN * 2 - 1, 'F') + "D"; // "FF..FFFFFFFFFFFFFD"
//string resp_bin_id = hex_to_chararray(resp_id);
//string resp_bin_prefix_id = hex_to_chararray(resp_prefix_id);
//
//void handleRequest(char *request, int /*request_len*/) {
//    unsigned char request_type;
//    unsigned char strategy;
//    unsigned int str_opt_len;
//    void *str_opt;
//    unsigned int no_publishers;
//    unsigned int no_subscribers;
//    unsigned char no_ids;
//    unsigned char IDLength;
//    unsigned int total_ids_length = 0;
//    set<string> publishers;
//    set<string> subscribers;
//    set<string> ids;
//    unsigned int idx = 0;
//    map<string, Bitvector *> result = map<string, Bitvector *>();
//    map<string, Bitvector *>::iterator map_iter;
//
//    unsigned char *response;
//    unsigned int response_size;
//    unsigned char response_type;
//    unsigned int response_idx = 0;
//    Bitvector *FIDToDestination;
//    string destination_node;
//    string response_id;
//
//    request_type = (unsigned char) *request;
//    strategy = (unsigned char) *(request + sizeof (request_type));
//    str_opt_len = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy));
//    /*str_opt is not allocated - be careful*/
//    str_opt = request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len);
//
//    if (request_type == MATCH_PUB_SUBS) {
//        cout << "MATCH_PUB_SUBS" << endl;
//        no_publishers = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len);
//        cout << "publishers: ";
//        for (unsigned int i = 0; i < no_publishers; i++) {
//            publishers.insert(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + idx, PURSUIT_ID_LEN));
//            cout << string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + idx, PURSUIT_ID_LEN) << " ";
//            idx += PURSUIT_ID_LEN;
//        }
//        cout << endl;
//        no_subscribers = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + idx);
//        cout << "subscribers: ";
//        for (unsigned int i = 0; i < no_subscribers; i++) {
//            subscribers.insert(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN));
//            cout << string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN) << " ";
//            idx += PURSUIT_ID_LEN;
//        }
//        cout << endl;
//        no_ids = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + idx);
//        cout << "IDs: ";
//        for (unsigned int i = 0; i < no_ids; i++) {
//            IDLength = (unsigned char) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + sizeof (no_ids) + idx);
//            ids.insert(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + sizeof (no_ids) + sizeof (IDLength) + idx, ((unsigned int) IDLength) * PURSUIT_ID_LEN));
//            cout << chararray_to_hex(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_publishers) + sizeof (no_subscribers) + sizeof (no_ids) + sizeof (IDLength) + idx, ((unsigned int) IDLength) * PURSUIT_ID_LEN)) << " ";
//            idx += (sizeof (IDLength) + IDLength * PURSUIT_ID_LEN);
//            total_ids_length += IDLength * PURSUIT_ID_LEN;
//        }
//        cout << endl;
//        tm_igraph->calculateFID(publishers, subscribers, result);
//        /*notify publishers*/
//        for (map_iter = result.begin(); map_iter != result.end(); map_iter++) {
//            response_idx = 0;
//            if ((*map_iter).second == NULL) {
//                response_size = sizeof (no_ids) + ((unsigned int) no_ids) * sizeof (IDLength) + total_ids_length + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (response_type);
//                response_type = STOP_PUBLISH;
//            } else {
//                response_size = sizeof (no_ids) + ((unsigned int) no_ids) * sizeof (IDLength) + total_ids_length + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (response_type) + FID_LEN;
//                response_type = START_PUBLISH;
//            }
//            response = (unsigned char *) malloc(response_size);
//            memcpy(response, &no_ids, sizeof (no_ids));
//            for (set<string>::iterator set_it = ids.begin(); set_it != ids.end(); set_it++) {
//                IDLength = (*set_it).length() / PURSUIT_ID_LEN;
//                memcpy(response + sizeof (no_ids) + response_idx, &IDLength, sizeof (IDLength));
//                memcpy(response + sizeof (no_ids) + response_idx + sizeof (IDLength), (*set_it).c_str(), (*set_it).length());
//                response_idx += sizeof (IDLength)+(*set_it).length();
//            }
//            memcpy(response + sizeof (no_ids) + response_idx, &strategy, sizeof (strategy));
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy), &str_opt_len, sizeof (str_opt_len));
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy) + sizeof (str_opt_len), str_opt, str_opt_len);
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len, &response_type, sizeof (response_type));
//            if ((*map_iter).second == NULL) {
//                /*do nothing*/
//            } else {
//                memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (response_type), (*map_iter).second->_data, FID_LEN);
//            }
//            /*find the FID to the publisher*/
//            destination_node = (*map_iter).first;
//            FIDToDestination = tm_igraph->calculateFID(tm_igraph->nodeID, destination_node);
//            response_id = resp_bin_prefix_id + (*map_iter).first;
//            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) FIDToDestination->_data, FID_LEN, response, response_size);
//            delete FIDToDestination;
//            free(response);
//            if ((*map_iter).second == NULL) {
//                /*do nothing*/
//            } else {
//                delete (*map_iter).second;
//            }
//        }
//    } else if ((request_type == SCOPE_PUBLISHED) || (request_type == SCOPE_UNPUBLISHED)) {
//        cout << "SCOPE_PUBLISHED or SCOPE_UNPUBLISHED" << endl;
//        /*this a request to notify subscribers about a new scope*/
//        no_subscribers = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len);
//        cout << "Subscribers: ";
//        for (unsigned int i = 0; i < no_subscribers; i++) {
//            subscribers.insert(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN));
//            cout << string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + idx, PURSUIT_ID_LEN) << " ";
//            idx += PURSUIT_ID_LEN;
//        }
//        cout << endl;
//        no_ids = (unsigned int) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + idx);
//        cout << "IDs: ";
//        for (unsigned int i = 0; i < no_ids; i++) {
//            IDLength = (unsigned char) *(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + sizeof (no_ids) + idx);
//            ids.insert(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + sizeof (no_ids) + sizeof (IDLength) + idx, ((unsigned int) IDLength) * PURSUIT_ID_LEN));
//            cout << chararray_to_hex(string(request + sizeof (request_type) + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (no_subscribers) + sizeof (no_ids) + sizeof (IDLength) + idx, ((unsigned int) IDLength) * PURSUIT_ID_LEN)) << " ";
//            idx += (sizeof (IDLength) + IDLength * PURSUIT_ID_LEN);
//            total_ids_length += IDLength * PURSUIT_ID_LEN;
//        }
//        cout << endl;
//        for (set<string>::iterator subscribers_it = subscribers.begin(); subscribers_it != subscribers.end(); subscribers_it++) {
//            response_idx = 0;
//	    response_size = sizeof (no_ids) + ((unsigned int) no_ids) * sizeof (IDLength) + total_ids_length + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (response_type);
//            response = (unsigned char *) malloc(response_size);
//            memcpy(response, &no_ids, sizeof (no_ids));
//            for (set<string>::iterator set_it = ids.begin(); set_it != ids.end(); set_it++) {
//                IDLength = (*set_it).length() / PURSUIT_ID_LEN;
//                memcpy(response + sizeof (no_ids) + response_idx, &IDLength, sizeof (IDLength));
//                memcpy(response + sizeof (no_ids) + response_idx + sizeof (IDLength), (*set_it).c_str(), (*set_it).length());
//                response_idx += sizeof (IDLength)+(*set_it).length();
//            }
//            memcpy(response + sizeof (no_ids) + response_idx, &strategy, sizeof (strategy));
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy), &str_opt_len, sizeof (str_opt_len));
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy) + sizeof (str_opt_len), str_opt, str_opt_len);
//            response_type = request_type;
//            memcpy(response + sizeof (no_ids) + response_idx + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len, &response_type, sizeof (response_type));
//            /*find the FID to the subscriber*/
//            destination_node = *subscribers_it;
//            FIDToDestination = tm_igraph->calculateFID(tm_igraph->nodeID, destination_node);
//            response_id = resp_bin_prefix_id + destination_node;
//            ba->publish_data(response_id, IMPLICIT_RENDEZVOUS, (char *) FIDToDestination->_data, FID_LEN, response, response_size);
//            delete FIDToDestination;
//            free(response);
//        }
//    }
//}
//
//void *event_listener_loop(void *arg) {
//    Blackadder *ba = (Blackadder *) arg;
//    while (listening) {
//        Event ev;
//        ba->getEvent(ev);
//        if (ev.type == PUBLISHED_DATA) {
//            handleRequest((char *) ev.data, ev.data_len);
//        } else if (ev.type == UNDEF_EVENT && !listening) {
//            cout << "TM: final event" << endl;
//        } else {
//            cout << "TM: I am not expecting any other notification...FATAL" << endl;
//        }
//    }
//    return NULL;
//}
//
//void sigfun(int /*sig*/) {
//    (void) signal(SIGINT, SIG_DFL);
//    listening = 0;
//    if (event_listener) {
//        pthread_cancel(*event_listener);
//    }
//    ba->disconnect();
//    delete ba;
//    exit(0);
//}

int
main (int argc, char* argv[])
{

  /* a shared_ptr to a network struct to populate using the topology file */
  network_ptr net_ptr (new network ());

  /* a boost bidirectional, directed graph representing the whole topology */
  network_graph_ptr net_graph_ptr (new network_graph (net_ptr));

  /* topology full file path */
  string topology_file;

  bool verbose = false;

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("topology-manager options");

  desc.add_options () ("help,h", "Print help message");
  desc.add_options () ("topology_file,t", boost::program_options::value<string> (&topology_file)->required (), "Topology file (required)");
  desc.add_options () ("verbose,v", "Print Network and Graph structures (Default: false)");

  /* parse command line arguments */
  try {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) {
      cout << desc << endl;
      return EXIT_SUCCESS;
    }

    if (vm.count ("verbose")) verbose = true;

    boost::program_options::notify (vm);
  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << endl << desc << endl;
    return EXIT_FAILURE;
  }

  /* read the topology */
  read_topology (net_graph_ptr, topology_file);

//    (void) signal(SIGINT, sigfun);
//    cout << "TM: starting - process ID: " << getpid() << endl;
//    if (argc != 2) {
//        cout << "TM: the topology file is missing" << endl;
//        exit(0);
//    }
//    tm_igraph = new tm_graph();
//    /*read the graphML file that describes the topology*/
//    if (tm_igraph->readTopology(argv[1]) < 0) {
//        cout << "TM: couldn't read topology file...aborting" << endl;
//        exit(0);
//    }
//    cout << "Blackadder Node: " << tm_igraph->nodeID << endl;
//    /***************************************************/
//    if (tm_igraph->mode.compare("kernel") == 0) {
//        ba = Blackadder::Instance(false);
//    } else {
//        ba = Blackadder::Instance(true);
//    }
//    pthread_create(&_event_listener, NULL, event_listener_loop, (void *) ba);
//    event_listener = &_event_listener;
//    ba->subscribe_scope(req_bin_id, req_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);
//    pthread_join(*event_listener, NULL);
//
//    cout << "TM: disconnecting" << endl;
//    ba->disconnect();
//    delete ba;
//    delete tm_igraph;
//    cout << "TM: exiting" << endl;
  return 0;
}
