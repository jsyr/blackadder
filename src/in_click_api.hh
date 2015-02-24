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

#ifndef CLICK_IN_CLICK_API_HH
#define CLICK_IN_CLICK_API_HH

#include <click/config.h>
#include <click/packet.hh>
#include <click/vector.hh>

#include "helper.hh"

CLICK_DECLS

static const unsigned char protocol = 0;

/**
 * @brief (Blackadder Core) A class with static methods to create Click packets according to the base pub/sub API
 * 
 * @TODO: check if I can reuse the event code to create event packets
 */
class InClickAPI {
public:
    InClickAPI();
    ~InClickAPI();
    static WritablePacket* prepare_publish_scope(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_publish_info(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_unpublish_scope(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_unpublish_info(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_subscribe_scope(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_subscribe_info(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_unsubscribe_scope(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_unsubscribe_info(unsigned int local_identifier,const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* create_packet(unsigned int local_identifier, unsigned char type, const String &id, const String &prefix_id, char strategy, void *str_opt, unsigned int str_opt_len);
    static WritablePacket* prepare_publish_data(unsigned int local_identifier, const String &id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, void *data, unsigned int data_len);
    static WritablePacket* prepare_publish_data(unsigned int local_identifier, const String &id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, unsigned int data_len);
    static WritablePacket* prepare_event(unsigned int local_identifier, unsigned char type, const String &id, unsigned int data_len);
    static WritablePacket* prepare_event(unsigned int local_identifier, unsigned char type, const String &id, Packet* existing_packet);
    static WritablePacket* prepare_network_publication(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, unsigned int data_len);
    static WritablePacket* prepare_network_publication(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, Packet* existing_packet);
    static void add_data(WritablePacket* packet, const void *data, unsigned int data_len);
};

CLICK_ENDDECLS
#endif
