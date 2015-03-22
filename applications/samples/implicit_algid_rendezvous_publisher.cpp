/*
 * Copyright (C) 2010-2011  George Parisis and Dirk Trossen
 * All rights reserved.
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
#include <signal.h>
#include <blackadder.h>

blackadder *ba;

int payload_size = 1400;
char *payload = (char *) malloc(payload_size);
char *end_payload = (char *) malloc(payload_size);

using namespace std;

void *event_listener_loop(void *arg) {
    blackadder *ba = (blackadder *) arg;
    string id = "5600000000000000";
    string last_id = "8800000000000000";
    string prefix_id = "AA00000000000000";

    string bin_id = hex_to_chararray(id);
    string bin_last_id = hex_to_chararray(last_id);
    string bin_prefix_id = hex_to_chararray(prefix_id);

    string final_bin_alg_id = bin_prefix_id + bin_id;
    string final_last_bin_alg_id = bin_prefix_id + bin_last_id;
    event ev;
    ba->get_event(ev);
    if (ev.type == START_PUBLISH) {
        cout << "start publishing " << endl;
        for (int i = 0; i < 100000; i++) {
            //cout << "publishing data for ID " << chararray_to_hex(final_bin_alg_id) << " with algorithmic ID" << chararray_to_hex(ev.id) << endl;
            ba->publish_data(final_bin_alg_id, IMPLICIT_RENDEZVOUS_ALGID_DOMAIN, (void*) ev.id.c_str(), ev.id.length(), payload, payload_size);
        }
        for (int i = 0; i < 1000; i++) {
            //cout << "publishing end flag for ID " << chararray_to_hex(ev.id) << endl;
            ba->publish_data(final_last_bin_alg_id, IMPLICIT_RENDEZVOUS_ALGID_DOMAIN, (void*) ev.id.c_str(), ev.id.length(), end_payload, payload_size);
        }
    }
    return NULL;
}

void sigfun(int sig) {
    (void) signal(SIGINT, SIG_DFL);
    cout << "disconnecting..." << endl;
    free(payload);
    free(end_payload);
    delete ba;
    exit(0);
}

int main(int argc, char* argv[]) {
    pthread_t event_listener;
    memset(payload, 'A', payload_size);
    memset(end_payload, 'B', payload_size);
    (void) signal(SIGINT, sigfun);
    if (argc > 1) {
        int user_or_kernel = atoi(argv[1]);
        if (user_or_kernel == 0) {
            ba = blackadder::instance(true);
        } else {
            ba = blackadder::instance(false);
        }
    } else {
        /*By Default I assume blackadder is running in user space*/
        ba = blackadder::instance(true);
    }
    cout << "Process ID: " << getpid() << endl;
    pthread_create(&event_listener, NULL, event_listener_loop, (void *) ba);
    string id = string(PURSUIT_ID_LEN * 2, '0'); // "0000000000000000"
    string prefix_id = string();
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, prefix_id, DOMAIN_LOCAL, NULL, 0);

    id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    prefix_id = string(PURSUIT_ID_LEN * 2, '0'); // "0000000000000000"
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);

    ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);

    pthread_join(event_listener, NULL);
    cout << "disconnecting" << endl;
    sleep(1);
    free(payload);
    free(end_payload);
    delete ba;
    return 0;
}
