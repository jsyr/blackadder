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
#include "lipsin_forwarding.hh"

CLICK_DECLS

LipsinForwarding::LipsinForwarding(Forwarder *_forwarder_element) {
    forwarder_element = _forwarder_element;
}

LipsinForwarding::~LipsinForwarding() {
    Vector<ForwardingEntry *>::iterator it;
    for (it = fwTable.begin(); it != fwTable.end(); it++) {
        delete (*it);
    }
}

int LipsinForwarding::addForwardingEntry(Vector<String> &conf) {
    int port;
    BABitvector *link_identifier;
    String network_type;
    ForwardingEntry *entry;
    cp_integer(conf[0], &port);
    conf.pop_front();
    cp_string(conf[0], &network_type);
    conf.pop_front();
    if (network_type.compare(String("MAC")) == 0) {
        String str_link_identifier;
        EtherAddress * source_address = new EtherAddress();
        if (cp_ethernet_address(conf[0], source_address) == false) {
            click_chatter("LipsinForwarding: malformed source MAC Address - aborting");
            delete source_address;
            return -1;
        }
        conf.pop_front();
        EtherAddress * destination_address = new EtherAddress();
        if (cp_ethernet_address(conf[0], destination_address) == false) {
            click_chatter("LipsinForwarding: malformed destination MAC Address - aborting");
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        cp_string(conf[0], &str_link_identifier);
        if (str_link_identifier.length() != FID_LEN * 8) {
            click_chatter("LipsinForwarding: LIPSIN identifier should be %d bytes long...it is %d bytes", FID_LEN * 8, str_link_identifier.length());
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        link_identifier = new BABitvector(FID_LEN * 8);
        for (int j = 0; j < str_link_identifier.length(); j++) {
            if (str_link_identifier.at(j) == '1') {
                (*link_identifier)[str_link_identifier.length() - j - 1] = true;
            } else {
                (*link_identifier)[str_link_identifier.length() - j - 1] = false;
            }
        }
        entry = new ForwardingEntry(DOMAIN_LOCAL, port, MAC, source_address, destination_address, link_identifier);
        fwTable.push_back(entry);
        click_chatter("LipsinForwarding: Click Port: %d, Network Type: %s, Source Ethernet Address: %s, Destination Ethernet Address: %s, LIPSIN Identifier:", port, network_type.c_str(), source_address->unparse().c_str(), destination_address->unparse().c_str());
        click_chatter("%s", link_identifier->to_string().c_str());
    } else if (network_type.compare(String("IP")) == 0) {
        String str_link_identifier;
        IPAddress *source_address = new IPAddress();
        IPAddress *destination_address = new IPAddress();
        if (cp_ip_address(conf[0], source_address) == false) {
            click_chatter("LipsinForwarding: malformed source IP Address - aborting");
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        if (cp_ip_address(conf[0], destination_address) == false) {
            click_chatter("LipsinForwarding: malformed destination IP Address - aborting");
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        cp_string(conf[0], &str_link_identifier);
        if (str_link_identifier.length() != FID_LEN * 8) {
            click_chatter("LipsinForwarding: LIPSIN identifier should be %d bytes long...it is %d bytes", FID_LEN * 8, str_link_identifier.length());
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        link_identifier = new BABitvector(FID_LEN * 8);
        for (int j = 0; j < str_link_identifier.length(); j++) {
            if (str_link_identifier.at(j) == '1') {
                (*link_identifier)[str_link_identifier.length() - j - 1] = true;
            } else {
                (*link_identifier)[str_link_identifier.length() - j - 1] = false;
            }
        }
        entry = new ForwardingEntry(DOMAIN_LOCAL, port, IP, source_address, destination_address, link_identifier);
        fwTable.push_back(entry);
        click_chatter("LipsinForwarding: Click Port: %d, Network Type: %s, Source IP Address: %s, Destination IP Address: %s, LIPSIN Identifier:", port, network_type.c_str(), source_address->unparse().c_str(), destination_address->unparse().c_str());
        click_chatter("%s", str_link_identifier.c_str());
    } else if (network_type.compare(String("INTERNAL")) == 0) {
        /*this is the internal link identifier*/
        String str_internal_link_identifier;
        cp_string(conf[0], &str_internal_link_identifier);
        if (str_internal_link_identifier.length() != FID_LEN * 8) {
            click_chatter("LipsinForwarding: internal LIPSIN identifier should be %d bytes long...it is %d bytes", FID_LEN * 8, str_internal_link_identifier.length());
            return -1;
        }
        conf.pop_front();
        link_identifier = new BABitvector(FID_LEN * 8);
        for (int j = 0; j < str_internal_link_identifier.length(); j++) {
            if (str_internal_link_identifier.at(j) == '1') {
                (*link_identifier)[str_internal_link_identifier.length() - j - 1] = true;
            } else {
                (*link_identifier)[str_internal_link_identifier.length() - j - 1] = false;
            }
        }
        entry = new ForwardingEntry(DOMAIN_LOCAL, port, INTERNAL_LINK, NULL, NULL, link_identifier);
        fwTable.push_back(entry);
        click_chatter("LipsinForwarding: Click Port: %d, Internal LIPSIN Identifier:", port);
        click_chatter("%s", str_internal_link_identifier.c_str());
    } else {
        click_chatter("LipsinForwarding: Network type %s is not supported - aborting");
        return -1;
    }
    return 0;
}

void LipsinForwarding::forwardPublicationFromNode(Packet *p) { 
    click_ip *post_ip;
    click_udp *post_udp;
    click_ether *post_ether;
    WritablePacket *finalPacket = NULL;
    WritablePacket *payload = NULL;
    BABitvector *link_identifier;
    unsigned char strategy;
    unsigned int forwarding_information_length;
    const void *forwarding_information;
    unsigned csum;
    uint16_t len;
    ForwardingEntry *entry;
    Vector<ForwardingEntry *> out_links;
    BABitvector FID(FID_LEN * 8);
    BABitvector andVector(FID_LEN * 8);
    Vector<ForwardingEntry *>::iterator out_links_it;
    int clone_counter = 1;
    strategy = *(p->data());
    memcpy(&forwarding_information_length, p->data() + sizeof (strategy), sizeof (forwarding_information_length));
    forwarding_information = p->data() + sizeof (strategy) + sizeof (forwarding_information_length);
    memcpy(FID._data, forwarding_information, forwarding_information_length);
    /*Check all entries in my forwarding table and forward appropriately*/
    for (int i = 0; i < fwTable.size(); i++) {
        entry = fwTable[i];
        link_identifier = (BABitvector *) entry->forwarding_information;
        andVector = (FID)&(*link_identifier);
        if (andVector == (*link_identifier)) {
            out_links.push_back(entry);
        }
    }
    if (out_links.size() == 0) {
        p->kill();
    } else {
        for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
            entry = *out_links_it;
            if (clone_counter == out_links.size()) {
                payload = p->uniqueify();
            } else {
                payload = p->clone()->uniqueify();
            }
            switch (entry->network_type) {
                case MAC:
                    /*add the Ethernet header and send*/
                    finalPacket = payload->push(sizeof (click_ether));
                    post_ether = reinterpret_cast<click_ether *> (finalPacket->data());
                    /*destination MAC*/
                    memcpy(post_ether->ether_dhost, ((EtherAddress *) entry->destination_address)->data(), MAC_LEN);
                    /*source MAC*/
                    memcpy(post_ether->ether_shost, ((EtherAddress *) entry->source_address)->data(), MAC_LEN);
                    /*protocol type 0x080a*/
                    post_ether->ether_type = forwarder_element->proto_type;
                    /*push the packet to the appropriate ToDevice Element*/
                    //click_chatter("LipsinForwarding: sending packet to mac address: %s - source address: %s - packet size: %d - using Click Port %d", EtherAddress(post_ether->ether_dhost).unparse().c_str(), EtherAddress(post_ether->ether_shost).unparse().c_str(), finalPacket->length(), entry->port);
                    break;
                case IP:
                    /*add the IP header and send*/
                    finalPacket = payload->push(sizeof (click_udp) + sizeof (click_ip));
                    post_ip = reinterpret_cast<click_ip *> (finalPacket->data());
                    post_udp = reinterpret_cast<click_udp *> (post_ip + 1);
                    // set up IP header
                    post_ip->ip_v = 4;
                    post_ip->ip_hl = sizeof (click_ip) >> 2;
                    post_ip->ip_len = htons(finalPacket->length());
                    post_ip->ip_id = htons(forwarder_element->_id.fetch_and_add(1));
                    post_ip->ip_p = IP_PROTO_UDP;
                    post_ip->ip_src = ((IPAddress *) entry->source_address)->in_addr();
                    post_ip->ip_dst = ((IPAddress *) entry->destination_address)->in_addr();
                    post_ip->ip_tos = 0;
                    post_ip->ip_off = 0;
                    post_ip->ip_ttl = 250;
                    post_ip->ip_sum = 0;
                    post_ip->ip_sum = click_in_cksum((unsigned char *) post_ip, sizeof (click_ip));
                    finalPacket->set_ip_header(post_ip, sizeof (click_ip));
                    // set up UDP header
                    post_udp->uh_sport = htons(55555);
                    post_udp->uh_dport = htons(55555);
                    len = finalPacket->length() - sizeof (click_ip);
                    post_udp->uh_ulen = htons(len);
                    post_udp->uh_sum = 0;
                    csum = click_in_cksum((unsigned char *) post_udp, len);
                    post_udp->uh_sum = click_in_cksum_pseudohdr(csum, post_ip, len);
                    //click_chatter("LipsinForwarding: sending packet with source IP address %s to IP address: %s", ((IPAddress *) entry->source_address)->unparse().c_str(), ((IPAddress *) entry->destination_address)->unparse().c_str());
                    break;
                case INTERNAL_LINK:
                    //click_chatter("LipsinForwarding: sending back to dispatcher..that is now correct and valid");
                    payload->pull(sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length);
                    /*putting strategy in the beginning again after removing the forwarding information*/
                    finalPacket = payload->push(sizeof (strategy));
                    memcpy(finalPacket->data(), &strategy, sizeof (strategy));
                    break;
                case SIM_DEVICE:
                    click_chatter("LipsinForwarding: TODO SIM_DEVICE");
                    break;
            }
            forwarder_element->output(entry->port).push(finalPacket);
            clone_counter++;
        }
    }
}

/*I NEED TO ELIMINATE SOME FALSE POSITIVES BY NOT FORWARDING WHEN THE SRC AND DST ADDRESSES ARE REVERSED*/
void LipsinForwarding::forwardPublicationFromNetwork(Packet *p, int network_type) {
    click_ip *pre_ip;
    click_udp *pre_udp;
    click_ether *pre_ether;
    click_ip *post_ip;
    click_udp *post_udp;
    click_ether *post_ether;
    WritablePacket *finalPacket = NULL;
    WritablePacket *payload = NULL;
    BABitvector *link_identifier;
    unsigned char strategy;
    unsigned int forwarding_information_length;
    const void *forwarding_information;
    ForwardingEntry *entry;
    Vector<ForwardingEntry *> out_links;
    BABitvector FID(FID_LEN * 8);
    BABitvector andVector(FID_LEN * 8);
    Vector<ForwardingEntry *>::iterator out_links_it;
    int clone_counter = 1;
    unsigned csum;
    uint16_t len;
    switch (network_type) {
        case MAC:
            pre_ether = reinterpret_cast<click_ether *> ((unsigned char *) p->data());
            p->pull(sizeof (click_ether));
            break;
        case IP:
            pre_ip = reinterpret_cast<click_ip *> ((unsigned char *) p->data());
            pre_udp = reinterpret_cast<click_udp *> (pre_ip + 1);
            p->pull(sizeof (click_udp) + sizeof (click_ip));
            break;
        case INTERNAL_LINK:
            click_chatter("LipsinForwarding: the network type can never be INTERNAL_LINK....instead the forwardPublicationFromNode method must have been called");
            break;
        case SIM_DEVICE:
            click_chatter("LipsinForwarding: TODO SIM_DEVICE");
            break;
    }
    strategy = *(p->data());
    memcpy(&forwarding_information_length, p->data() + sizeof (strategy), sizeof (forwarding_information_length));
    forwarding_information = p->data() + sizeof (strategy) + sizeof (forwarding_information_length);
    memcpy(FID._data, forwarding_information, forwarding_information_length);
    /*Check all entries in my forwarding table and forward appropriately*/
    for (int i = 0; i < fwTable.size(); i++) {
        entry = fwTable[i];
        link_identifier = (BABitvector *) entry->forwarding_information;
        andVector = (FID)&(*link_identifier);
        if (andVector == (*link_identifier)) {
            /*check that I am not sending using reverse src and dst addresses*/
            /*****************************************************************/
            out_links.push_back(entry);
        }
    }
    if (out_links.size() == 0) {
        p->kill();
    } else {
        for (out_links_it = out_links.begin(); out_links_it != out_links.end(); out_links_it++) {
            entry = *out_links_it;
            if (clone_counter == out_links.size()) {
                payload = p->uniqueify();
            } else {
                payload = p->clone()->uniqueify();
            }
            switch (entry->network_type) {
                case MAC:
                    /*add the Ethernet header and send*/
                    finalPacket = payload->push_mac_header(sizeof (click_ether));
                    post_ether = reinterpret_cast<click_ether *> (finalPacket->data());
                    /*prepare the mac header*/
                    /*destination MAC*/
                    memcpy(post_ether->ether_dhost, ((EtherAddress *) entry->destination_address)->data(), MAC_LEN);
                    /*source MAC*/
                    memcpy(post_ether->ether_shost, ((EtherAddress *) entry->source_address)->data(), MAC_LEN);
                    /*protocol type 0x080a*/
                    post_ether->ether_type = forwarder_element->proto_type;
                    /*push the packet to the appropriate ToDevice Element*/
                    //click_chatter("LipsinForwarding: sending packet to mac address: %s - source address: %s - packet size: %d - using Click Port %d", EtherAddress(post_ether->ether_dhost).unparse().c_str(), EtherAddress(post_ether->ether_shost).unparse().c_str(), finalPacket->length(), entry->port);
                    break;
                case IP:
                    /*add the IP header and send*/
                    finalPacket = payload->push(sizeof (click_udp) + sizeof (click_ip));
                    post_ip = reinterpret_cast<click_ip *> (finalPacket->data());
                    post_udp = reinterpret_cast<click_udp *> (post_ip + 1);
                    // set up IP header
                    post_ip->ip_v = 4;
                    post_ip->ip_hl = sizeof (click_ip) >> 2;
                    post_ip->ip_len = htons(finalPacket->length());
                    post_ip->ip_id = htons(forwarder_element->_id.fetch_and_add(1));
                    post_ip->ip_p = IP_PROTO_UDP;
                    post_ip->ip_src = ((IPAddress *) entry->source_address)->in_addr();
                    post_ip->ip_dst = ((IPAddress *) entry->destination_address)->in_addr();
                    post_ip->ip_tos = 0;
                    post_ip->ip_off = 0;
                    post_ip->ip_ttl = 250;
                    post_ip->ip_sum = 0;
                    post_ip->ip_sum = click_in_cksum((unsigned char *) post_ip, sizeof (click_ip));
                    finalPacket->set_ip_header(post_ip, sizeof (click_ip));
                    // set up UDP header
                    post_udp->uh_sport = htons(55555);
                    post_udp->uh_dport = htons(55555);
                    len = finalPacket->length() - sizeof (click_ip);
                    post_udp->uh_ulen = htons(len);
                    post_udp->uh_sum = 0;
                    csum = click_in_cksum((unsigned char *) post_udp, len);
                    post_udp->uh_sum = click_in_cksum_pseudohdr(csum, post_ip, len);
                    //click_chatter("LipsinForwarding: sending packet to IP address: %s", ((IPAddress *) entry->destination_address)->unparse().c_str());
                    break;
                case INTERNAL_LINK:
                    payload->pull(sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length);
                    /*putting strategy in the beginning again after removing the forwarding information*/
                    finalPacket = payload->push(sizeof (strategy));
                    memcpy(finalPacket->data(), &strategy, sizeof (strategy));
                    break;
                case SIM_DEVICE:
                    click_chatter("LipsinForwarding: TODO SIM_DEVICE");
                    break;
            }
            forwarder_element->output(entry->port).push(finalPacket);
            clone_counter++;
        }
    }
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(LipsinForwarding)
