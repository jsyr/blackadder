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
#include "local_handler_interface.hh"

CLICK_DECLS

LocalHandlerInterface::~LocalHandlerInterface() {
    
}

void LocalHandlerInterface::publishDataLocally(LocalHostStringHashMap &localSubscribers, Packet *p /*the packet has some headroom and only the data*/) {
    unsigned int counter = 1;
    unsigned int localSubscribersSize = localSubscribers.size();
    /*only local subscribers exist*/
    for (LocalHostStringHashMapIter localSubscribers_it = localSubscribers.begin(); localSubscribers_it != localSubscribers.end(); localSubscribers_it++) {
        if (counter == localSubscribersSize) {
            /*don't clone the packet since this is the last subscriber - use the correct information identifier*/
            dispatcher_element->pushDataEvent((*localSubscribers_it).first->id, (*localSubscribers_it).second, p);
        } else {
            /*use the correct information identifier*/
            dispatcher_element->pushDataEvent((*localSubscribers_it).first->id, (*localSubscribers_it).second, p->clone()->uniqueify());
        }
        counter++;
    }
}

LocalHost * LocalHandlerInterface::getLocalHost(int id, PubSubIdx &local_pub_sub_Index) {
    LocalHost *_localhost;
    _localhost = local_pub_sub_Index.get(id);
    if (_localhost == local_pub_sub_Index.default_value()) {
        _localhost = new LocalHost(id);
        local_pub_sub_Index.set(id, _localhost);
    }
    return _localhost;
}

bool LocalHandlerInterface::findLocalSubscribers(Vector<String> &IDs, ActiveSubIdx &activeSubscriptionIndex, LocalHostStringHashMap & _localSubscribers) {
    bool foundSubscribers;
    String knownID;
    LocalHostSetIter set_it;
    Vector<String>::iterator id_it;
    ActiveSubscription *as;
    foundSubscribers = false;
    /*prefix-match checking here for all known IDS of aiip*/
    for (id_it = IDs.begin(); id_it != IDs.end(); id_it++) {
        knownID = *id_it;
        /*check for local subscription for the specific information item*/
        as = activeSubscriptionIndex.get(knownID);
        if (as != activeSubscriptionIndex.default_value()) {
            for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
                _localSubscribers.set((*set_it).pointer, knownID);
                foundSubscribers = true;
            }
        }
        /*check for local subscription for the father item*/
        as = activeSubscriptionIndex.get(knownID.substring(0, knownID.length() - PURSUIT_ID_LEN));
        if (as != activeSubscriptionIndex.default_value()) {
            for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
                _localSubscribers.set((*set_it).pointer, knownID);
                foundSubscribers = true;
            }
        }
    }
    return foundSubscribers;
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(LocalHandlerInterface)
