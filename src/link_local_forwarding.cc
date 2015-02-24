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

#include "link_local_forwarding.hh"

CLICK_DECLS

LinkLocalForwarding::LinkLocalForwarding(Forwarder *_forwarder_element) {
    forwarder_element = _forwarder_element;
}

LinkLocalForwarding::~LinkLocalForwarding() {
    Vector<ForwardingEntry *>::iterator it;
    for (it = fwTable.begin(); it != fwTable.end(); it++) {
        delete (*it);
    }
}

int LinkLocalForwarding::addForwardingEntry(Vector<String> &conf) {
    int port;
    String network_type;
    ForwardingEntry *entry;
    cp_integer(conf[0], &port);
    conf.pop_front();
    cp_string(conf[0], &network_type);
    conf.pop_front();
    if (network_type.compare(String("MAC")) == 0) {
        EtherAddress * source_address = new EtherAddress();
        if (cp_ethernet_address(conf[0], source_address) == false) {
            click_chatter("LinkLocalForwarding: malformed source MAC Address - aborting");
            delete source_address;
            return -1;
        }
        conf.pop_front();
        EtherAddress * destination_address = new EtherAddress();
        if (cp_ethernet_address(conf[0], destination_address) == false) {
            click_chatter("LinkLocalForwarding: malformed destination MAC Address - aborting");
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        entry = new ForwardingEntry(LINK_LOCAL, port, MAC, source_address, destination_address, NULL);
        fwTable.push_back(entry);
        click_chatter("LinkLocalForwarding: Click Port: %d, Network Type: %s, Source Ethernet Address: %s, Destination Ethernet Address: %s", port, network_type.c_str(), source_address->unparse().c_str(), destination_address->unparse().c_str());
    } else if (network_type.compare(String("IP")) == 0) {
        IPAddress *source_address = new IPAddress();
        if (cp_ip_address(conf[0], source_address) == false) {
            click_chatter("LinkLocalForwarding: malformed source IP Address - aborting");
            delete source_address;
            return -1;
        }
        conf.pop_front();
        IPAddress *destination_address = new IPAddress();
        if (cp_ip_address(conf[0], destination_address) == false) {
            click_chatter("LinkLocalForwarding: malformed destination IP Address - aborting");
            delete source_address;
            delete destination_address;
            return -1;
        }
        conf.pop_front();
        entry = new ForwardingEntry(LINK_LOCAL, port, IP, source_address, destination_address, NULL);
        fwTable.push_back(entry);
        click_chatter("LinkLocalForwarding: Click Port: %d, Network Type: %s, Source IP Address: %s, Destination IP Address: %s", port, network_type.c_str(), source_address->unparse().c_str(), destination_address->unparse().c_str());
    } else {
        click_chatter("LinkLocalForwarding: Network type %s is not supported - aborting");
        return -1;
    }
    return 0;
}

void LinkLocalForwarding::forwardPublicationFromNode(Packet *p) {
    Vector<ForwardingEntry *>::iterator it;
    bool shouldBreak = false;
    ForwardingEntry *entry;
    unsigned csum;
    uint16_t len;
    unsigned char strategy;
    unsigned int forwarding_information_length;
    const void *forwarding_information;
    WritablePacket *payload, *finalPacket = NULL;
    click_ip *ip;
    click_udp *udp;
    click_ether *ether;
    int counter = 1;
    strategy = *(p->data());
    memcpy(&forwarding_information_length, p->data() + sizeof (strategy), sizeof (forwarding_information_length));
    /*from the Dispatcher - send only to the network*/
    switch (strategy) {
        case LINK_LOCAL:
            if (forwarding_information_length > 0) {
                forwarding_information = p->data() + sizeof (strategy) + sizeof (forwarding_information_length);
                for (it = fwTable.begin(); it != fwTable.end(); it++) {
                    entry = *it;
                    switch (entry->network_type) {
                        case MAC:
                            if ((forwarding_information_length == MAC_LEN) && (memcmp(forwarding_information, ((EtherAddress *) entry->destination_address)->data(), MAC_LEN) == 0)) {
                                shouldBreak = true;
                            }
                            break;
                        case IP:
                            if ((forwarding_information_length == IP_LEN) && (memcmp(forwarding_information, ((IPAddress *) entry->destination_address)->data(), IP_LEN) == 0)) {
                                shouldBreak = true;
                            }
                            break;
                        case INTERNAL_LINK:
                            click_chatter("LinkLocalForwarding: INTERNAL_LINK not possible...");
                            break;
                        case SIM_DEVICE:
                            click_chatter("LinkLocalForwarding: TODO SIM_DEVICE");
                            break;
                    }
                    if (shouldBreak == true) {
                        break;
                    }
                }
                if (it != fwTable.end()) {
                    /*that means I jumped from a break point above..use the entry to forward*/
                    switch (entry->network_type) {
                        case MAC:
                            /*add the Ethernet header and send*/
                            finalPacket = p->push(sizeof (click_ether));
                            ether = reinterpret_cast<click_ether *> (finalPacket->data());
                            /*prepare the mac header*/
                            /*destination MAC*/
                            memcpy(ether->ether_dhost, ((EtherAddress *) entry->destination_address)->data(), MAC_LEN);
                            /*source MAC*/
                            memcpy(ether->ether_shost, ((EtherAddress *) entry->source_address)->data(), MAC_LEN);
                            /*protocol type 0x080a*/
                            ether->ether_type = forwarder_element->proto_type;
                            /*push the packet to the appropriate ToDevice Element*/
                            //click_chatter("sending packet to mac address: %s - source address: %s - packet size: %d - using Click Port %d", EtherAddress(ether->ether_dhost).unparse().c_str(), EtherAddress(ether->ether_shost).unparse().c_str(), finalPacket->length(), entry->port);
                            break;
                        case IP:
                            /*add the IP header and send*/
                            finalPacket = p->push(sizeof (click_udp) + sizeof (click_ip));
                            ip = reinterpret_cast<click_ip *> (finalPacket->data());
                            udp = reinterpret_cast<click_udp *> (ip + 1);
                            // set up IP header
                            ip->ip_v = 4;
                            ip->ip_hl = sizeof (click_ip) >> 2;
                            ip->ip_len = htons(finalPacket->length());
                            ip->ip_id = htons(forwarder_element->_id.fetch_and_add(1));
                            ip->ip_p = IP_PROTO_UDP;
                            ip->ip_src = ((IPAddress *) entry->source_address)->in_addr();
                            ip->ip_dst = ((IPAddress *) entry->destination_address)->in_addr();
                            ip->ip_tos = 0;
                            ip->ip_off = 0;
                            ip->ip_ttl = 250;
                            ip->ip_sum = 0;
                            ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                            finalPacket->set_ip_header(ip, sizeof (click_ip));
                            // set up UDP header
                            udp->uh_sport = htons(55555);
                            udp->uh_dport = htons(55555);
                            len = finalPacket->length() - sizeof (click_ip);
                            udp->uh_ulen = htons(len);
                            udp->uh_sum = 0;
                            csum = click_in_cksum((unsigned char *) udp, len);
                            udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                            //click_chatter("sending packet to IP address: %s", ((IPAddress *) entry->destination_address)->unparse().c_str());
                            break;
                        case INTERNAL_LINK:
                            click_chatter("LinkLocalForwarding: INTERNAL_LINK not possible...");
                            break;
                        case SIM_DEVICE:
                            click_chatter("LinkLocalForwarding: TODO SIM_DEVICE");
                            break;
                    }
                    forwarder_element->output(entry->port).push(finalPacket);
                } else {
                    click_chatter("LinkLocalForwarding: LINK_LOCAL strategy - did not find a matching entry - killing the packet");
                    p->kill();
                }
            } else {
                click_chatter("LinkLocalForwarding: LINK_LOCAL strategy needs some forwarding information..");
                p->kill();
            }
            break;
        case BROADCAST_IF:
            if (fwTable.empty()) {
                p->kill();
            } else {
                for (it = fwTable.begin(); it != fwTable.end(); it++) {
                    entry = *it;
                    if (counter == fwTable.size()) {
                        payload = p->uniqueify();
                    } else {
                        payload = p->clone()->uniqueify();
                    }
                    switch (entry->network_type) {
                        case MAC:
                            /*add the Ethernet header and send*/
                            finalPacket = payload->push(sizeof (click_ether));
                            ether = reinterpret_cast<click_ether *> (finalPacket->data());
                            /*prepare the mac header*/
                            /*destination MAC*/
                            memcpy(ether->ether_dhost, ((EtherAddress *) entry->destination_address)->data(), MAC_LEN);
                            /*source MAC*/
                            memcpy(ether->ether_shost, ((EtherAddress *) entry->source_address)->data(), MAC_LEN);
                            /*protocol type 0x080a*/
                            ether->ether_type = forwarder_element->proto_type;
                            /*push the packet to the appropriate ToDevice Element*/
                            //click_chatter("sending packet to mac address: %s - source address: %s - packet size: %d - using Click Port %d", EtherAddress(ether->ether_dhost).unparse().c_str(), EtherAddress(ether->ether_shost).unparse().c_str(), finalPacket->length(), entry->port);
                            break;
                        case IP:
                            /*add the IP header and send*/
                            finalPacket = payload->push(sizeof (click_udp) + sizeof (click_ip));
                            ip = reinterpret_cast<click_ip *> (finalPacket->data());
                            udp = reinterpret_cast<click_udp *> (ip + 1);
                            // set up IP header
                            ip->ip_v = 4;
                            ip->ip_hl = sizeof (click_ip) >> 2;
                            ip->ip_len = htons(finalPacket->length());
                            ip->ip_id = htons(forwarder_element->_id.fetch_and_add(1));
                            ip->ip_p = IP_PROTO_UDP;
                            ip->ip_src = ((IPAddress *) entry->source_address)->in_addr();
                            ip->ip_dst = ((IPAddress *) entry->destination_address)->in_addr();
                            ip->ip_tos = 0;
                            ip->ip_off = 0;
                            ip->ip_ttl = 250;
                            ip->ip_sum = 0;
                            ip->ip_sum = click_in_cksum((unsigned char *) ip, sizeof (click_ip));
                            finalPacket->set_ip_header(ip, sizeof (click_ip));
                            // set up UDP header
                            udp->uh_sport = htons(55555);
                            udp->uh_dport = htons(55555);
                            len = finalPacket->length() - sizeof (click_ip);
                            udp->uh_ulen = htons(len);
                            udp->uh_sum = 0;
                            csum = click_in_cksum((unsigned char *) udp, len);
                            udp->uh_sum = click_in_cksum_pseudohdr(csum, ip, len);
                            //click_chatter("sending packet to IP address: %s", ((IPAddress *) entry->destination_address)->unparse().c_str());
                            break;
                        case INTERNAL_LINK:
                            click_chatter("LinkLocalForwarding: INTERNAL_LINK not possible...");
                            break;
                        case SIM_DEVICE:
                            click_chatter("LinkLocalForwarding: TODO SIM_DEVICE");
                            break;
                    }
                    forwarder_element->output(entry->port).push(finalPacket);
                    counter++;
                }
            }
            break;
    }
}

void LinkLocalForwarding::forwardPublicationFromNetwork(Packet *p, int network_type) {
    unsigned char strategy;
    unsigned int forwarding_information_length;
    WritablePacket *finalPacket = NULL;
    switch (network_type) {
        case MAC:
            p->pull(sizeof (click_ether));
            break;
        case IP:
            p->pull(sizeof (click_udp) + sizeof (click_ip));
            break;
        case INTERNAL_LINK:
            click_chatter("LinkLocalForwarding: INTERNAL_LINK not possible...");
            break;
        case SIM_DEVICE:
            click_chatter("LinkLocalForwarding: TODO SIM_DEVICE");
            break;
    }
    strategy = *(p->data());
    memcpy(&forwarding_information_length, p->data() + sizeof (strategy), sizeof (forwarding_information_length));
    p->pull(sizeof (strategy) + sizeof (forwarding_information_length) + forwarding_information_length);
    /*putting strategy in the beginning again after removing the forwarding information*/
    finalPacket = p->push(sizeof (strategy));
    memcpy(finalPacket->data(), &strategy, sizeof (strategy));
    forwarder_element->output(0).push(finalPacket);
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(LinkLocalForwarding)
