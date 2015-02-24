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

#ifndef CLICK_LINKLOCALFORWARDING_HH
#define CLICK_LINKLOCALFORWARDING_HH

#include "forwarding_interface.hh"

CLICK_DECLS

class LinkLocalForwarding : public ForwardingInterface {
public:
    LinkLocalForwarding(Forwarder *_forwarder_element);
    ~LinkLocalForwarding();
    int addForwardingEntry(Vector<String> &conf);
    void forwardPublicationFromNode(Packet *p);
    void forwardPublicationFromNetwork(Packet *p, int network_type);
private:
    Vector<ForwardingEntry *> fwTable;
};

CLICK_ENDDECLS
#endif

