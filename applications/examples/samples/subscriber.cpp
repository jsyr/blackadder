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

#include "blackadder.h"
#include <signal.h>

blackadder *ba;

using namespace std;

void sigfun(int sig) {
    (void) signal(SIGINT, SIG_DFL);
    delete ba;
    exit(0);
}

int main(int argc, char* argv[]) {
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
    string id = "0000000000000000";
    string prefix_id;
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    cout << "Subscribing to Scope " << prefix_id << id << endl;
    ba->subscribe_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    while (true) {
        event ev;
        ba->get_event(ev);
        switch (ev.type) {
            case SCOPE_PUBLISHED:
                cout << "SCOPE_PUBLISHED: " << chararray_to_hex(ev.id) << endl;
                bin_id = ev.id.substr(ev.id.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                bin_prefix_id = ev.id.substr(0, ev.id.length() - PURSUIT_ID_LEN);
                cout << "Subscribing to Scope " << chararray_to_hex(ev.id) << endl;
                ba->subscribe_scope(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
                break;
            case SCOPE_UNPUBLISHED:
                cout << "SCOPE_UNPUBLISHED: " << chararray_to_hex(ev.id) << endl;
                break;
            case START_PUBLISH:
                cout << "START_PUBLISH: " << chararray_to_hex(ev.id) << endl;
                break;
            case STOP_PUBLISH:
                cout << "STOP_PUBLISH: " << chararray_to_hex(ev.id) << endl;
                break;
            case PUBLISHED_DATA:
                cout << "received data for item: " << chararray_to_hex(ev.id) << endl;
                break;
        }
    }
    delete ba;
    return 0;
}
