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

#include "forwarder.hh"
/*these must be inlcuded here and NOT In the .hh file*/
#include "link_local_forwarding.hh"
#include "lipsin_forwarding.hh"

/*****************************************************/

CLICK_DECLS

Forwarder::Forwarder() {
    unsigned int reverse_proto;
    cp_integer(String("0x080a"), 16, &reverse_proto);
    proto_type = htons(reverse_proto);
    link_local_forwarding = new LinkLocalForwarding(this);
    lipsin_forwarding = new LipsinForwarding(this);
}

Forwarder::~Forwarder() {
    click_chatter("Forwarder: destroyed!");
}

int Forwarder::configure(Vector<String> &conf, ErrorHandler *errh) {
    int number_of_ports;
    int port_number;
    String port_type;
    int number_of_links;
    int ret;
    int strategy;
    _id = 0;
    click_chatter("*****************************************************FORWARDER CONFIGURATION*****************************************************");
    cp_integer(conf[0], &number_of_ports);
    conf.pop_front();
    for (int i = 0; i < number_of_ports; i++) {
        cp_integer(conf[0], &port_number);
        conf.pop_front();
        cp_string(conf[0], &port_type);
        conf.pop_front();
        if (port_type.compare("MAC") == 0) {
            click_chatter("Forwarder: port %d interfaces to an Ethernet network", port_number);
            port_types.set(port_number, MAC);
        } else if (port_type.compare("IP") == 0) {
            click_chatter("Forwarder: port %d interfaces to an IP network", port_number);
            port_types.set(port_number, IP);
        } else if (port_type.compare("SIM_DEVICE") == 0) {
            click_chatter("Forwarder: port %d interfaces to a Simulated network", port_number);
            click_chatter("Forwarder: NS3 SIM_DEVICE not supported yet");
            port_types.set(port_number, SIM_DEVICE);
            return -1;
        }
    }
    cp_integer(conf[0], &number_of_links);
    conf.pop_front();
    click_chatter("Forwarder: Number of Links: %d", number_of_links);
    for (int i = 0; i < number_of_links; i++) {
        cp_integer(conf[0], &strategy);
        conf.pop_front();
        switch (strategy) {
            case LINK_LOCAL:
                click_chatter("Forwarder: adding forwarding entry for LINK_LOCAL strategy");
                ret = link_local_forwarding->addForwardingEntry(conf);
                if (ret < 0) {
                    errh->error("could not add forwarding entry..aborting...");
                    return -1;
                }
                break;
            case DOMAIN_LOCAL:
                click_chatter("Forwarder: adding forwarding entry for DOMAIN_LOCAL strategy");
                ret = lipsin_forwarding->addForwardingEntry(conf);
                if (ret < 0) {
                    errh->error("could not add forwarding entry..aborting...");
                    return -1;
                }
                break;
            default:
                errh->error("Forwarder: I don't know this strategy..aborting...");
                return -1;
        }
    }
    return 0;
}

int Forwarder::initialize(ErrorHandler */*errh*/) {
    return 0;
}

void Forwarder::cleanup(CleanupStage /*stage*/) {
    delete link_local_forwarding;
    delete lipsin_forwarding;
}

void Forwarder::push(int in_port, Packet *p) {
    unsigned char strategy = 0;
    int port_type;
    if (in_port == 0) {
        strategy = *(p->data());
        switch (strategy) {
            case LINK_LOCAL:
                link_local_forwarding->forwardPublicationFromNode(p);
                break;
            case DOMAIN_LOCAL:
                lipsin_forwarding->forwardPublicationFromNode(p);
                break;
            case IMPLICIT_RENDEZVOUS:
                lipsin_forwarding->forwardPublicationFromNode(p);
                break;
            case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
                lipsin_forwarding->forwardPublicationFromNode(p);
                break;
            case BROADCAST_IF:
                link_local_forwarding->forwardPublicationFromNode(p);
                break;
            default:
                click_chatter("Forwarder: Unknown strategy %d - don't know what to do..", strategy);
                p->kill();
                break;
        }
    } else {
        /*depending on the type of the network device, get and pull header and call the respective strategy handling*/
        port_type = port_types.get(in_port);
        switch (port_type) {
            case MAC:
                strategy = *(p->data() + sizeof (click_ether));
                break;
            case IP:
                strategy = *(p->data() + sizeof (click_udp) + sizeof (click_ip));
                break;
            case SIM_DEVICE:
                strategy = 0;
                click_chatter("Forwarder: FromSimDevice: TODO");
                p->kill();
                return;
        }
        switch (strategy) {
            case LINK_LOCAL:
                link_local_forwarding->forwardPublicationFromNetwork(p, port_type);
                break;
            case DOMAIN_LOCAL:
                lipsin_forwarding->forwardPublicationFromNetwork(p, port_type);
                break;
            case IMPLICIT_RENDEZVOUS:
                lipsin_forwarding->forwardPublicationFromNetwork(p, port_type);
                break;
            case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
                lipsin_forwarding->forwardPublicationFromNetwork(p, port_type);
                break;
            case BROADCAST_IF:
                link_local_forwarding->forwardPublicationFromNetwork(p, port_type);
                break;
            default:
                click_chatter("Forwarder: Unknown strategy %d - don't know what to do..", strategy);
                p->kill();
                break;
        }
    }
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Forwarder)

