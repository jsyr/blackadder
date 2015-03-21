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

#include <blackadder.h>
#include <signal.h>

blackadder *ba;
int payload_size = 1400;
char *payload = (char *) malloc(payload_size);
char *end_payload = (char *) malloc(payload_size);

void sigfun(int /*sig*/) {
    (void) signal(SIGINT, SIG_DFL);
    free(payload);
    delete ba;
    exit(0);
}

int main(int argc, char* argv[]) {
    string id, prefix_id, bin_id, bin_prefix_id, final_bin_id;
    (void) signal(SIGINT, sigfun);
    memset(payload, 'A', payload_size);
    memset(end_payload, 'B', payload_size);
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
    prefix_id = "0000000000000000";
    id = "1111111111111111";
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    final_bin_id = bin_prefix_id + bin_id;
    cout << "Publishing using broadcast strategy" << chararray_to_hex(final_bin_id) << endl;

    for (int i = 0; i < 100000; i++) {
        ba->publish_data(final_bin_id, BROADCAST_IF, NULL, 0, payload, payload_size);
    }
    for (int i = 0; i < 1000; i++) {
        ba->publish_data(final_bin_id, BROADCAST_IF, NULL, 0, end_payload, payload_size);
    }

    free(payload);
    free(end_payload);
    delete ba;
    return 0;
}
