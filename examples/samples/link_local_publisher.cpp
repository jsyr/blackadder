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
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef __linux__
#include <netinet/ether.h>
#else
#include <netinet/if_ether.h>
#endif

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
    int ret;
    bool not_mac = false;
    bool not_ip = false;
    struct ether_addr *dest_mac_address;
    struct in_addr dest_ip_address;
    string id, prefix_id, bin_id, bin_prefix_id, final_bin_id;
    unsigned int forwarding_information_length;
    if (argc < 2) {
        cerr << "please provide mac or ip address" << endl;
        exit(-1);
    }
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
    if ((dest_mac_address = ether_aton(argv[1])) == 0) {
        not_mac = true;
    } else {
        forwarding_information_length = 6;
    }
    if ((ret = inet_aton(argv[1], &dest_ip_address)) == 0) {
        not_ip = true;
    } else {
        forwarding_information_length = 4;
    }
    if (not_ip && not_mac) {
        cerr << "please provide a valid mac or ip address" << endl;
        exit(-1);
    } else if (not_mac == false) {
        cout << "Publishing using link-local strategy with identifier: " << chararray_to_hex(final_bin_id) << endl;
        for (int i = 0; i < 100000; i++) {
            ba->publish_data(final_bin_id, LINK_LOCAL, (void *) dest_mac_address->ether_addr_octet, forwarding_information_length, payload, payload_size);
        }
        for (int i = 0; i < 1000; i++) {
            ba->publish_data(final_bin_id, LINK_LOCAL, (void *) dest_mac_address->ether_addr_octet, forwarding_information_length, end_payload, payload_size);
        }
    } else if (not_ip == false) {
        cout << "Publishing using link-local strategy with identifier: " << chararray_to_hex(final_bin_id) << endl;
        for (int i = 0; i < 100000; i++) {
            ba->publish_data(final_bin_id, LINK_LOCAL, (void *) &dest_ip_address.s_addr, forwarding_information_length, payload, payload_size);
        }
        for (int i = 0; i < 1000; i++) {
            ba->publish_data(final_bin_id, LINK_LOCAL, (void *) &dest_ip_address.s_addr, forwarding_information_length, end_payload, payload_size);
        }
    }

    free(payload);
    free(end_payload);
    ba->disconnect();
    delete ba;
    return 0;
}
