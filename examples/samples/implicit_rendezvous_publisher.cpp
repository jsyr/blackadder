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

#include <blackadder.hpp>
#include <bitvector.hpp>
#include <signal.h>

Blackadder *ba;
int payload_size = 1400;
char *payload = (char *) malloc(payload_size);
char *end_payload = (char *) malloc(payload_size);

void sigfun(int /*sig*/) {
    (void) signal(SIGINT, SIG_DFL);
    ba->disconnect();
    free(payload);
    delete ba;
    exit(0);
}

int main(int argc, char* argv[]) {
    string id, prefix_id, bin_id, bin_prefix_id, final_bin_id;
    string forwarding_identifier;
    Bitvector lipsin_identifier(FID_LEN * 8);
    if (argc < 2) {
        cerr << "please provide forwarding identifier" << endl;
        exit(-1);
    }
    if (strlen(argv[1]) != FID_LEN * 8) {
        cout << "wrong size of forwarding identifier" << endl;
        exit(0);
    }
    forwarding_identifier = string(argv[1], FID_LEN * 8);
    memset(payload, 'A', payload_size);
    memset(end_payload, 'B', payload_size);
    (void) signal(SIGINT, sigfun);
    if (argc > 2) {
        int user_or_kernel = atoi(argv[2]);
        if (user_or_kernel == 0) {
            ba = Blackadder::Instance(true);
        } else {
            ba = Blackadder::Instance(false);
        }
    } else {
        /*By Default I assume blackadder is running in user space*/
        ba = Blackadder::Instance(true);
    }
    cout << "Process ID: " << getpid() << endl;
    prefix_id = "0000000000000000";
    id = "1111111111111111";
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    final_bin_id = bin_prefix_id + bin_id;
    for (int j = 0; j < forwarding_identifier.length(); j++) {
        if (forwarding_identifier.at(j) == '1') {
            lipsin_identifier[forwarding_identifier.length() - j - 1] = true;
        } else {
            lipsin_identifier[forwarding_identifier.length() - j - 1] = false;
        }
    }
    cout << "Publishing using implicit rendezvous strategy with forwarding identifier: " << lipsin_identifier.to_string() << endl;
    for (int i = 0; i < 100000; i++) {
        ba->publish_data(final_bin_id, IMPLICIT_RENDEZVOUS, lipsin_identifier._data, FID_LEN, payload, payload_size);
    }
    for (int i = 0; i < 1000; i++) {
        ba->publish_data(final_bin_id, IMPLICIT_RENDEZVOUS, lipsin_identifier._data, FID_LEN, end_payload, payload_size);
    }
    free(payload);
    free(end_payload);
    ba->disconnect();
    delete ba;
    return 0;
}
