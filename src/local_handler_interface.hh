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

#ifndef CLICK_LOCALHANDLERINTERFACE_HH
#define CLICK_LOCALHANDLERINTERFACE_HH

#include <click/config.h>
#include <click/packet.hh>
#include <click/string.hh>
#include <click/hashtable.hh>

#include "common.hh"
#include "helper.hh"
#include "ba_bitvector.hh"
#include "activepub.hh"
#include "in_click_api.hh"
#include "localhost.hh"
#include "dispatcher.hh"

CLICK_DECLS

/** @brief A Click's HashTable of pointers to LocalHost mapped to Click's Strings.
 */
typedef HashTable<LocalHost *, String> LocalHostStringHashMap;
/** @brief An iterator to a Click's HashTable of pointers to LocalHost mapped to Click's Strings.
 */
typedef LocalHostStringHashMap::iterator LocalHostStringHashMapIter;
/** @brief A set (implemented as a Click's HashTable) of applications and click elements (see localhost.hh).
 */
typedef HashTable<PointerSetItem<LocalHost> > LocalHostSet;
/** @brief An iterator to a set (implemented as a Click's HashTable) of applications and click elements (see localhost.hh).
 */
typedef LocalHostSet::iterator LocalHostSetIter;
/** @brief A Click's HashTable of integers mapped to pointers of LocalHost.
 */
typedef HashTable<int, LocalHost *> PubSubIdx;
/** @brief An iterator to a Click's HashTable of integers mapped to pointers of LocalHost.
 */
typedef PubSubIdx::iterator PubSubIdxIter;
/** @brief A Click's HashTable of Click's Strings mapped to an ActivePublication.
 */
typedef HashTable<String, ActivePublication *> ActivePubIdx;
/** @brief An iterator to a Click's HashTable of Click's Strings mapped to an ActivePublication.
 */
typedef ActivePubIdx::iterator ActivePubIter;
/** @brief A Click's HashTable of Click's Strings mapped to an ActiveSubscription.
 */
typedef HashTable<String, ActiveSubscription *> ActiveSubIdx;
/** @brief An iterator to a Click's HashTable of Click's Strings mapped to an ActiveSubscription.
 */
typedef ActiveSubIdx::iterator ActiveSubIter;

class LocalHandlerInterface {
public:
    virtual ~LocalHandlerInterface();
    virtual void handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char &type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual void handleLocalPublication(Packet *p, unsigned int local_identifier, String &ID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual void handleNetworkPublication(Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/) = 0;
    virtual void handleRVNotification(Vector<String> IDs, unsigned char strategy, unsigned int str_opt_len, const void *str_opt, Packet *p) = 0;
    virtual void handleLocalDisconnection(unsigned int local_identifier) = 0;
    /*should be common in most strategies - override if a different behaviour is required*/
    virtual void publishDataLocally(LocalHostStringHashMap &localSubscribers, Packet *p /*the packet has some headroom and only the data*/);
    bool findLocalSubscribers(Vector<String> &IDs, ActiveSubIdx &activeSubscriptionIndex, LocalHostStringHashMap & _localSubscribers);
    LocalHost * getLocalHost(int id, PubSubIdx &local_pub_sub_Index);
    
    Dispatcher *dispatcher_element;
};

CLICK_ENDDECLS
#endif

