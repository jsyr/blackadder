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
#include "forwarding_interface.hh"

CLICK_DECLS

ForwardingEntry::ForwardingEntry(unsigned char _strategy, int _port, unsigned char _network_type, void *_source_address, void *_destination_address, void *_forwarding_information) {
    strategy = _strategy;
    port = _port;
    network_type = _network_type;
    source_address = _source_address;
    destination_address = _destination_address;
    forwarding_information = _forwarding_information;
}

ForwardingEntry::~ForwardingEntry() {
    EtherAddress * mac_source_address;
    EtherAddress * mac_destination_address;
    IPAddress *ip_source_address;
    IPAddress *ip_destination_address;
    BABitvector *link_identifier;
    switch (network_type) {
        case MAC:
            mac_source_address = (EtherAddress *) source_address;
            mac_destination_address = (EtherAddress *) destination_address;
            delete mac_source_address;
            delete mac_destination_address;
            switch (strategy) {
                case LINK_LOCAL:
                    break;
                case DOMAIN_LOCAL:
                    link_identifier = (BABitvector *) forwarding_information;
                    delete link_identifier;
                    break;
            }
            break;
        case IP:
            ip_source_address = (IPAddress *) source_address;
            ip_destination_address = (IPAddress *) destination_address;
            delete ip_source_address;
            delete ip_destination_address;
            switch (strategy) {
                case LINK_LOCAL:
                    break;
                case DOMAIN_LOCAL:
                    link_identifier = (BABitvector *) forwarding_information;
                    delete link_identifier;
                    break;
            }
            break;
        case INTERNAL_LINK:
            switch (strategy) {
                case LINK_LOCAL:
                    click_chatter("ForwardingEntry: there is no internal link for the LINK_LOCAL strategy...something is wrong");
                    break;
                case DOMAIN_LOCAL:
                    link_identifier = (BABitvector *) forwarding_information;
                    delete link_identifier;
                    break;
            }
            break;
        case SIM_DEVICE:
            click_chatter("ForwardingEntry: TODO SIM_DEVICE");
            break;
        default:
            click_chatter("ForwardingEntry: unknown network type..don't know how to destruct entry");
            break;
    }
}

ForwardingInterface::~ForwardingInterface() {

}

CLICK_ENDDECLS
ELEMENT_PROVIDES(ForwardingInterface)
ELEMENT_PROVIDES(ForwardingEntry)
