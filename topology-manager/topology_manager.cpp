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

#include "topology-manager.h"

using namespace std;

/* a shared_ptr to a network struct to populate using the topology file */
network_ptr net_ptr (new network ());

/* a boost bidirectional, directed graph representing the whole topology */
network_graph_ptr net_graph_ptr (new network_graph (net_ptr));

/* topology full file path */
string topology_file;

bool verbose = false;
bool is_kernelspace = false;

boost::shared_ptr<blackadder> ba;

pthread_t _event_listener, *event_listener = NULL;
sig_atomic_t listening = 1;

string req_id = string (PURSUIT_ID_LEN * 2 - 1, 'F') + "E"; // "FF..FFFFFFFFFFFFFE"
string req_prefix_id;
string req_bin_id = hex_to_chararray (req_id);
string req_bin_prefix_id = hex_to_chararray (req_prefix_id);

string resp_id;
string resp_prefix_id = string (PURSUIT_ID_LEN * 2 - 1, 'F') + "D"; // "FF..FFFFFFFFFFFFFD"
string resp_bin_id = hex_to_chararray (resp_id);
string resp_bin_prefix_id = hex_to_chararray (resp_prefix_id);

void
handle_match_pub_sub_request (char *match_request, unsigned char request_type, unsigned char strategy, unsigned int str_opt_len, const char *str_opt)
{
  char *temp_buffer = match_request;
  unsigned int no_publishers, no_subscribers, total_ids_length = 0, response_size = 0;
  unsigned char no_ids, id_len, response_type;
  set<string> publishers, subscribers, ids;

  /* will contain all publishers with a NULL or not NULL lipsin identifier to one or more subscribers */
  map<string, boost::shared_ptr<bitvector> > result;

  boost::shared_ptr<bitvector> lipsin_to_dst;
  string response_id;

  cout << "topology-manager: topology creation for matching publishers with subscribers" << endl;

  no_publishers = (unsigned int) *(temp_buffer);
  temp_buffer += sizeof(no_publishers);
  cout << "publishers: ";
  for (unsigned int i = 0; i < no_publishers; i++) {
    string publisher (temp_buffer, PURSUIT_ID_LEN);
    temp_buffer += PURSUIT_ID_LEN;
    publishers.insert (publisher);
    cout << publisher << " ";
  }
  cout << endl;

  no_subscribers = (unsigned int) *(temp_buffer);
  temp_buffer += sizeof(no_subscribers);
  cout << "subscribers: ";
  for (unsigned int i = 0; i < no_subscribers; i++) {
    string subscriber (temp_buffer, PURSUIT_ID_LEN);
    temp_buffer += PURSUIT_ID_LEN;
    subscribers.insert (subscriber);
    cout << subscriber << " ";
  }
  cout << endl;

  no_ids = (unsigned char) *(temp_buffer);
  temp_buffer += sizeof(no_ids);
  cout << "IDs: ";
  for (unsigned int i = 0; i < (unsigned int) no_ids; i++) {
    id_len = (unsigned char) *(temp_buffer);
    temp_buffer += sizeof(id_len);
    string id (temp_buffer, ((unsigned int) id_len) * PURSUIT_ID_LEN);
    temp_buffer += (unsigned int) id_len;
    ids.insert (id);
    cout << chararray_to_hex (id) << " ";
    total_ids_length += id_len * PURSUIT_ID_LEN;
  }
  cout << endl;

  match_pubs_subs (publishers, subscribers, result);

  /*notify publishers*/
  BOOST_FOREACH(result_map_iter result_pair, result) {
    char *response, *temp_response;

    /* publisher to be notified */
    string publisher = result_pair.first;

    /* lipsin identifier to be communicated to the publisher */
    boost::shared_ptr<bitvector> lipsin_ptr = result_pair.second;

    if (!lipsin_ptr) {
      /* if shared pointer to lipsin identifier is NULL, publish STOP_PUBLISH */
      response_size = sizeof(no_ids) + ((unsigned int) no_ids) * sizeof(id_len) + total_ids_length + sizeof(strategy) + sizeof(str_opt_len) + str_opt_len + sizeof(response_type);
      response_type = STOP_PUBLISH;
    } else {
      /* else publish START_PUBLISH along with the lipisn identifier to be used */
      response_size = sizeof(no_ids) + ((unsigned int) no_ids) * sizeof(id_len) + total_ids_length + sizeof(strategy) + sizeof(str_opt_len) + str_opt_len + sizeof(response_type) + FID_LEN;
      response_type = START_PUBLISH;
    }

    response = (char *) malloc (response_size);
    temp_response = response;

    memcpy (temp_response, &no_ids, sizeof(no_ids));
    temp_response += sizeof(no_ids);
    BOOST_FOREACH(string id, ids) {
      id_len = id.length () / PURSUIT_ID_LEN;
      memcpy (temp_response, &id_len, sizeof(id_len));
      temp_response += sizeof(id_len);
      memcpy (temp_response, id.c_str (), id.length ());
      temp_response += id.length ();
    }

    memcpy (temp_response, &strategy, sizeof(strategy));
    temp_response += sizeof(strategy);
    memcpy (temp_response, &str_opt_len, sizeof(str_opt_len));
    temp_response += sizeof(str_opt_len);
    memcpy (temp_response, str_opt, str_opt_len);
    temp_response += str_opt_len;
    response_type = request_type;
    memcpy (temp_response, &response_type, sizeof(response_type));
    temp_response += sizeof(response_type);

    if (!lipsin_ptr) {
      /*do nothing*/
    } else {
      memcpy (temp_response, lipsin_ptr->_data, FID_LEN);
      temp_response += FID_LEN;
    }

    /*find the FID to the publisher*/
    lipsin_id_ptr lipsin_to_pub_ptr (new bitvector (FID_LEN * 8));
    shortest_path (topology_manager_label, publisher, lipsin_to_pub_ptr);

    response_id = resp_bin_prefix_id + publisher;
    ba->publish_data (response_id, IMPLICIT_RENDEZVOUS, (char *) lipsin_to_pub_ptr->_data, FID_LEN, response, response_size);

    free (response);
  }
}

void
handle_scope_request (char *scope_request, unsigned char request_type, unsigned char strategy, unsigned int str_opt_len, const char *str_opt)
{
  char *temp_buffer = scope_request;
  unsigned int no_subscribers = 0, total_ids_length = 0, response_size = 0;
  unsigned char no_ids, id_len, response_type;
  set<string> subscribers, ids;

  string response_id;

  cout << "topology-manager: topology creation for published or unpublished scope" << endl;

  no_subscribers = (unsigned int) (*temp_buffer);
  temp_buffer += sizeof(no_subscribers);
  cout << "Subscribers: ";
  for (unsigned int i = 0; i < no_subscribers; i++) {
    string subscriber (temp_buffer, PURSUIT_ID_LEN);
    temp_buffer += PURSUIT_ID_LEN;
    subscribers.insert (subscriber);
    cout << subscriber << " ";
  }
  cout << endl;

  no_ids = (unsigned char) *(temp_buffer);
  temp_buffer += sizeof(no_ids);
  cout << "IDs: ";
  for (unsigned int i = 0; i < (unsigned int) no_ids; i++) {
    id_len = (unsigned char) *(temp_buffer);
    temp_buffer += sizeof(id_len);
    string id (temp_buffer, ((unsigned int) id_len) * PURSUIT_ID_LEN);
    temp_buffer += (unsigned int) id_len;
    ids.insert (id);
    cout << chararray_to_hex (id) << " ";
    total_ids_length += id_len * PURSUIT_ID_LEN;
  }
  cout << endl;

  BOOST_FOREACH(string subscriber, subscribers) {
    char *response, *temp_response;
    response_size = sizeof(no_ids) + ((unsigned int) no_ids) * sizeof(id_len) + total_ids_length + sizeof(strategy) + sizeof(str_opt_len) + str_opt_len + sizeof(response_type);
    response = (char *) malloc (response_size);
    temp_response = response;

    memcpy (temp_response, &no_ids, sizeof(no_ids));
    temp_response += sizeof(no_ids);
    BOOST_FOREACH(string id, ids) {
      id_len = id.length () / PURSUIT_ID_LEN;
      memcpy (temp_response, &id_len, sizeof(id_len));
      temp_response += sizeof(id_len);
      memcpy (temp_response, id.c_str (), id.length ());
      temp_response += id.length ();
    }

    memcpy (temp_response, &strategy, sizeof(strategy));
    temp_response += sizeof(strategy);
    memcpy (temp_response, &str_opt_len, sizeof(str_opt_len));
    temp_response += sizeof(str_opt_len);
    memcpy (temp_response, str_opt, str_opt_len);
    temp_response += str_opt_len;
    response_type = request_type;
    memcpy (temp_response, &response_type, sizeof(response_type));
    temp_response += sizeof(response_type);

    /* find the forwarding identifier to the subscriber */
    lipsin_id_ptr lipsin_ptr (new bitvector (FID_LEN * 8));
    shortest_path (topology_manager_label, subscriber, lipsin_ptr);

    response_id = resp_bin_prefix_id + subscriber;
    ba->publish_data (response_id, IMPLICIT_RENDEZVOUS, (char *) lipsin_ptr->_data, FID_LEN, response, response_size);

    free (response);
  }
}

void
handle_request (char *request, int /*request_len*/)
{
  char *temp_buffer;
  unsigned char request_type;
  unsigned char strategy;
  unsigned int str_opt_len;
  const char *str_opt;

  temp_buffer = request;
  request_type = (unsigned char) *temp_buffer;
  temp_buffer += sizeof(request_type);
  strategy = (unsigned char) *(temp_buffer);
  temp_buffer += sizeof(strategy);
  str_opt_len = (unsigned int) *(temp_buffer);
  temp_buffer += sizeof(str_opt_len);
  str_opt = temp_buffer; /* str_opt is not allocated - be careful */
  temp_buffer += str_opt_len;
  if (request_type == MATCH_PUB_SUBS) {
    handle_match_pub_sub_request (temp_buffer, request_type, strategy, str_opt_len, str_opt);
  } else if ((request_type == SCOPE_PUBLISHED) || (request_type == SCOPE_UNPUBLISHED)) {
    handle_scope_request (temp_buffer, request_type, strategy, str_opt_len, str_opt);
  }
}

void *
event_listener_loop (void *arg)
{
  while (listening) {
    event ev;
    ba->get_event (ev);
    if (ev.type == PUBLISHED_DATA) {

      handle_request ((char *) ev.data, ev.data_len);

    } else if (ev.type == UNDEF_EVENT && !listening) {
      cout << "topology-manager: final event" << endl;
    } else {
      cerr << "topology-manager: I am not expecting any other notification..." << endl;
    }
  }
  return NULL;
}

void
signal_handler (int)
{
  (void) signal (SIGINT, SIG_DFL);
  listening = 0;
  if (event_listener) {
    pthread_cancel (*event_listener);
  }
}

int
main (int argc, char* argv[])
{

  boost::program_options::variables_map vm;

  boost::program_options::options_description desc ("topology-manager options");

  desc.add_options () ("help,h", "Print help message");
  desc.add_options () ("topology_file,t", boost::program_options::value<string> (&topology_file)->required (), "Topology file (required)");
  desc.add_options () ("verbose,v", "Print Network and Graph structures (Default: false)");
  desc.add_options () ("is_kernelspace,k", "is blackadder running in kernel space? (Default: false)");

  /* parse command line arguments */
  try {
    boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), vm);

    if (vm.count ("help")) {
      cout << desc << endl;
      return EXIT_SUCCESS;
    }

    if (vm.count ("verbose")) verbose = true;
    if (vm.count ("is_kernelspace")) is_kernelspace = true;

    boost::program_options::notify (vm);
  } catch (boost::program_options::error& e) {
    cerr << "ERROR: " << e.what () << endl << desc << endl;
    return EXIT_FAILURE;
  }

  /* load the network using the provided configuration file (boost property tree) */
  load_network (net_ptr, topology_file);

  /* create boost graph using the network constructed above */
  create_graph (net_graph_ptr, net_ptr);

  /* build forwarding base */
  build_forwarding_base (net_graph_ptr);

  if (verbose) {
    /* print forwarding base */
    print_forwarding_base (net_graph_ptr);
  }
  /* add signal handler to allow for graceful exits */
  (void) signal (SIGINT, signal_handler);
  ba = boost::shared_ptr<blackadder> (blackadder::instance (!is_kernelspace));

  cout << "topology-manager: node with label " << net_ptr->tm_node->label << " is running..." << endl;

  pthread_create (&_event_listener, NULL, event_listener_loop, NULL);
  event_listener = &_event_listener;

  cout << "topology-manager: subscribing to scope " << req_prefix_id + req_id << endl;
  ba->subscribe_scope (req_bin_id, req_bin_prefix_id, IMPLICIT_RENDEZVOUS, NULL, 0);

  pthread_join (*event_listener, NULL);

  cout << "topology-manager: exiting" << endl;

  return 0;
}
