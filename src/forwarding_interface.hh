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

#ifndef CLICK_FORWARDINGINTERFACE_HH
#define CLICK_FORWARDINGINTERFACE_HH

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/udp.h>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/vector.hh>
#include <click/packet.hh>

#include "helper.hh"
#include "ba_bitvector.hh"
#include "forwarder.hh"

CLICK_DECLS

/**@brief (Blackadder Core) a forwarding_entry represents an entry in the forwarding table of this Blackadder node.
 */
class ForwardingEntry {
public:
    ForwardingEntry(unsigned char _strategy, int _port, unsigned char _network_type, void *_source_address, void *_destination_address, void *_forwarding_information);
    ~ForwardingEntry();
    unsigned char strategy;
    unsigned char network_type;
    /**@brief the source address for this entry.
     */
    void *source_address;
    /**@brief the destination MAC address for this entry (or unused).
     */
    void *destination_address;
    /**@brief the output port for the network element that where packets should be forwarded when this entry is used.
     */
    int port;
    /**@brief
     */
    void *forwarding_information;
};

class ForwardingInterface {
public:
    virtual ~ForwardingInterface();
    virtual int addForwardingEntry(Vector<String> &conf) = 0;
    virtual void forwardPublicationFromNode(Packet *p) = 0;
    virtual void forwardPublicationFromNetwork(Packet *p, int network_type) = 0;
    Forwarder *forwarder_element;
};

CLICK_ENDDECLS
#endif

