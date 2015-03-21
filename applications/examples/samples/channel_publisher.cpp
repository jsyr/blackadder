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

int payload_size = 1000;
char *payload = (char *) malloc(payload_size);
char *end_payload = (char *) malloc(payload_size);
pthread_t event_listener;

using namespace std;

void *event_listener_loop(void *arg) {
    blackadder *ba = (blackadder *) arg;
    event ev;
    ba->get_event(ev);
    if (ev.type == START_PUBLISH) {
        cout << "start publishing " << endl;
        for (int i = 0; i < 100000; i++) {
            //cout << "publishing data for ID " << chararray_to_hex(ev.id) << endl;
            ba->publish_data(ev.id, DOMAIN_LOCAL, NULL, 0, payload, payload_size);
        }
        for (int i = 0; i < 1000; i++) {
            //cout << "publishing end flag for ID " << chararray_to_hex(ev.id) << endl;
            ba->publish_data(ev.id, DOMAIN_LOCAL, NULL, 0, end_payload, payload_size);
        }
    }
    return NULL;
}

void sigfun(int /*sig*/) {
    (void) signal(SIGINT, SIG_DFL);
    pthread_cancel(event_listener);
}

int main(int argc, char* argv[]) {
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
    string id = string(PURSUIT_ID_LEN*2, '1'); // "1111111111111111"
    string prefix_id = string();
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, prefix_id, DOMAIN_LOCAL, NULL, 0);

    id = string(PURSUIT_ID_LEN*2, '1'); // "1111111111111111"
    prefix_id = string(PURSUIT_ID_LEN*2, '1'); // "1111111111111111"
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
