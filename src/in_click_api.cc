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
#include "in_click_api.hh"

CLICK_DECLS

WritablePacket* InClickAPI::prepare_publish_scope(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_SCOPE request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_SCOPE request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_SCOPE request - id cannot be empty");
    } else {
        return create_packet(local_identifier, PUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_publish_info(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_INFO request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_INFO request - wrong prefix_id size");
    } else if (prefix_id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_INFO request - prefix_id cannot be empty");
    } else if (prefix_id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send PUBLISH_INFO request - prefix_id cannot be empty");
    } else {
        return create_packet(local_identifier, PUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_unpublish_scope(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_SCOPE request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_SCOPE request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_SCOPE request - id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_SCOPE request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, UNPUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_unpublish_info(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_INFO request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_INFO request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_INFO request - id cannot be empty");
    } else if (prefix_id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_INFO request - prefix_id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send UNPUBLISH_INFO request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, UNPUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_subscribe_scope(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_SCOPE request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_SCOPE request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_SCOPE request - id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_SCOPE request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, SUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_subscribe_info(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_INFO request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_INFO request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_INFO request - id cannot be empty");
    } else if (prefix_id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_INFO request - prefix_id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send SUBSCRIBE_INFO request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, SUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_unsubscribe_scope(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_SCOPE request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_SCOPE request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_SCOPE request - id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_SCOPE request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, UNSUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

WritablePacket* InClickAPI::prepare_unsubscribe_info(unsigned int local_identifier, const String &id, const String &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len) {
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_INFO request - wrong ID size");
    } else if (prefix_id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_INFO request - wrong prefix_id size");
    } else if (id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_INFO request - id cannot be empty");
    } else if (prefix_id.length() == 0) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_INFO request - prefix_id cannot be empty");
    } else if (id.length() / PURSUIT_ID_LEN > 1) {
        click_chatter("InClickAPI Library: Could not send UNSUBSCRIBE_INFO request - id cannot consist of multiple fragments");
    } else {
        return create_packet(local_identifier, UNSUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
    }
    return NULL;
}

/*nothing netlink socket related*/
WritablePacket* InClickAPI::prepare_publish_data(unsigned int local_identifier, const String &id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, void *data, unsigned int data_len) {
    WritablePacket *packet;
    unsigned int click_packet_length;
    unsigned char type = PUBLISH_DATA;
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send  - wrong ID size");
    } else {
        click_packet_length = sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len) + data_len;
        if (str_opt_len > 0) {
            click_packet_length += str_opt_len;
        }
        packet = Packet::make(click_packet_length);
        memcpy(packet->data(), &protocol, sizeof (protocol));
        memcpy(packet->data() + sizeof (protocol), &local_identifier, sizeof (local_identifier));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier), &type, sizeof (type));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type), &id_len, sizeof (id_len));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len), id.c_str(), id.length());
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length(), &strategy, sizeof (strategy));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy), &str_opt_len, sizeof (str_opt_len));
        if (str_opt_len > 0) {
            memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len), str_opt, str_opt_len);
            memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len, data, data_len);
        } else {
            memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len), data, data_len);
        }
        return packet;
    }
    return NULL;
}

/*nothing netlink socket related
 *In this version, actual data will be added one by one after this call will return.*/
WritablePacket* InClickAPI::prepare_publish_data(unsigned int local_identifier, const String &id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, unsigned int data_len) {
    WritablePacket *packet;
    unsigned int click_packet_length;
    unsigned char type = PUBLISH_DATA;
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    if (id.length() % PURSUIT_ID_LEN != 0) {
        click_chatter("InClickAPI Library: Could not send  - wrong ID size");
    } else {
        click_packet_length = sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len) + data_len;
        if (str_opt_len > 0) {
            click_packet_length += str_opt_len;
        }
        packet = Packet::make(100, NULL, click_packet_length - data_len, data_len);
        memcpy(packet->data(), &protocol, sizeof (protocol));
        memcpy(packet->data() + sizeof (protocol), &local_identifier, sizeof (local_identifier));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier), &type, sizeof (type));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type), &id_len, sizeof (id_len));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len), id.c_str(), id.length());
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length(), &strategy, sizeof (strategy));
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy), &str_opt_len, sizeof (str_opt_len));
        if (str_opt_len > 0) {
            memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (strategy) + sizeof (str_opt_len), str_opt, str_opt_len);
        }
        return packet;
    }
    return NULL;
}

/*nothing netlink socket related*/
WritablePacket* InClickAPI::create_packet(unsigned int local_identifier, unsigned char type, const String &id, const String &prefix_id, char strategy, void *str_opt, unsigned int str_opt_len) {
    unsigned int click_packet_length;
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    unsigned char prefix_id_len = prefix_id.length() / PURSUIT_ID_LEN;
    WritablePacket *packet;
    click_packet_length = sizeof (protocol) + sizeof (unsigned int) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (prefix_id_len) + prefix_id.length() + sizeof (strategy) + sizeof (str_opt_len);
    if (str_opt_len > 0) {
        click_packet_length += str_opt_len;
    }
    packet = Packet::make(100, NULL, click_packet_length, 100);
    memcpy(packet->data(), &protocol, sizeof (protocol));
    memcpy(packet->data() + sizeof (protocol), &local_identifier, sizeof (local_identifier));
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier), &type, sizeof (type));
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type), &id_len, sizeof (id_len));
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len), id.c_str(), id.length());
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length(), &prefix_id_len, sizeof (prefix_id_len));
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (prefix_id_len), prefix_id.c_str(), prefix_id.length());
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (prefix_id_len) + prefix_id.length(), &strategy, sizeof (strategy));
    memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (prefix_id_len) + prefix_id.length() + sizeof (strategy), &str_opt_len, sizeof (str_opt_len));
    if (str_opt_len > 0) {
        memcpy(packet->data() + sizeof (protocol) + sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + sizeof (prefix_id_len) + prefix_id.length() + sizeof (strategy) + sizeof (str_opt_len), str_opt, str_opt_len);
    }
    return packet;
}

WritablePacket* InClickAPI::prepare_event(unsigned int local_identifier, unsigned char type, const String &id, unsigned int data_len) {
    WritablePacket *event_packet;
    unsigned int click_packet_length;
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    /***********************************************************/
    click_packet_length = sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length() + data_len;
    event_packet = Packet::make(100, NULL, click_packet_length - data_len, data_len);
    memcpy(event_packet->data(), &local_identifier, sizeof (local_identifier));
    memcpy(event_packet->data() + sizeof (local_identifier), &type, sizeof (type));
    memcpy(event_packet->data() + sizeof (local_identifier) + sizeof (type), &id_len, sizeof (id_len));
    memcpy(event_packet->data() + sizeof (local_identifier) + sizeof (type) + sizeof (id_len), id.c_str(), id.length());
    return event_packet;
}

WritablePacket* InClickAPI::prepare_event(unsigned int local_identifier, unsigned char type, const String &id, Packet* existing_packet) {
    WritablePacket *event_packet;
    unsigned char id_len = id.length() / PURSUIT_ID_LEN;
    /***********************************************************/
    event_packet = existing_packet->push(sizeof (local_identifier) + sizeof (type) + sizeof (id_len) + id.length());
    memcpy(event_packet->data(), &local_identifier, sizeof (local_identifier));
    memcpy(event_packet->data() + sizeof (local_identifier), &type, sizeof (type));
    memcpy(event_packet->data() + sizeof (local_identifier) + sizeof (type), &id_len, sizeof (id_len));
    memcpy(event_packet->data() + sizeof (local_identifier) + sizeof (type) + sizeof (id_len), id.c_str(), id.length());
    return event_packet;
}

WritablePacket* InClickAPI::prepare_network_publication(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, unsigned int data_len) {
    WritablePacket *publication_packet;
    unsigned int click_packet_length;
    Vector<String>::iterator it;
    unsigned int index = 0;
    unsigned char numberOfIDs = IDs.size();
    /***********************************************************/
    click_packet_length = sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + sizeof (numberOfIDs);
    for (it = IDs.begin(); it != IDs.end(); it++) {
        click_packet_length += sizeof (unsigned char);
        click_packet_length += (*it).length();
    }
    click_packet_length += data_len;
    publication_packet = Packet::make(100, NULL, click_packet_length - data_len, data_len);
    memcpy(publication_packet->data(), &strategy, sizeof (strategy));
    memcpy(publication_packet->data() + sizeof (strategy), &forwarding_information_length, sizeof (forwarding_information_length));
    memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length), forwarding_information, forwarding_information_length);
    memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length, &numberOfIDs, sizeof (numberOfIDs));
    for (it = IDs.begin(); it != IDs.end(); it++) {
        unsigned char id_len = ((*it).length()) / PURSUIT_ID_LEN;
        memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + numberOfIDs + index, &id_len, sizeof (id_len));
        memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + numberOfIDs + index + sizeof (id_len), (*it).c_str(), (*it).length());
        index += sizeof (id_len) + (*it).length();
    }
    return publication_packet;
}

WritablePacket* InClickAPI::prepare_network_publication(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, Packet* existing_packet) {
    WritablePacket *publication_packet;
    unsigned int header_length;
    unsigned int index = 0;
    unsigned char numberOfIDs = IDs.size();
    Vector<String>::iterator it;
    header_length = sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + sizeof (numberOfIDs);
    for (it = IDs.begin(); it != IDs.end(); it++) {
        header_length += sizeof (unsigned char);
        header_length += (*it).length();
    }
    publication_packet = existing_packet->push(header_length);
    /*these two may overlap..pointing at the same exact point in memory*/
    memcpy(publication_packet->data(), &strategy, sizeof (strategy));
    memcpy(publication_packet->data() + sizeof (strategy), &forwarding_information_length, sizeof (forwarding_information_length));
    memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length), forwarding_information, forwarding_information_length);
    memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length, &numberOfIDs, sizeof (numberOfIDs));
    for (it = IDs.begin(); it != IDs.end(); it++) {
        unsigned char id_len = ((*it).length()) / PURSUIT_ID_LEN;
        memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + sizeof (numberOfIDs) + index, &id_len, sizeof (id_len));
        memcpy(publication_packet->data() + sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length + sizeof (numberOfIDs) + index + sizeof (id_len), (*it).c_str(), (*it).length());
        index += sizeof (id_len) + (*it).length();
    }
    return publication_packet;
}

void InClickAPI::add_data(WritablePacket* packet, const void *data, unsigned int data_len) {
    unsigned int current_index = packet->length();
    packet = packet->put(data_len);
    memcpy(packet->data() + current_index, data, data_len);
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(InClickAPI)
