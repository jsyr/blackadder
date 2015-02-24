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

#ifndef CLICK_RENDEZVOUSINTERFACE_HH
#define CLICK_RENDEZVOUSINTERFACE_HH

#include <click/config.h>
#include <click/string.hh>
#include <click/hashtable.hh>

#include "common.hh"
#include "in_click_api.hh"
#include "scope.hh"
#include "informationitem.hh"
#include "remotehost.hh"
#include "ba_bitvector.hh"
#include "rv.hh"

CLICK_DECLS

/** @brief A set (implemented as a Click's HashTable) of Click's Strings.
 */
typedef HashTable<ObjectSetItem<String> > StringSet;
/** @brief An iterator to a set (implemented as a Click's HashTable) of Click's Strings.
 */
typedef StringSet::iterator StringSetIter;
/** @brief A set (implemented as a Click's HashTable) of Remote Hosts (see remotehost.hh).
 */
typedef HashTable<PointerSetItem<RemoteHost> > RemoteHostSet;
/** @brief An iterator to a set (implemented as a Click's HashTable) of Remote Hosts (see remotehost.hh).
 */
typedef RemoteHostSet::iterator RemoteHostSetIter;
/** @brief A Click's HashTable of Click's Strings mapped to pointers to Scopes.
 */
typedef HashTable<String, Scope *> ScopeHashMap;
/** @brief An iterator Click's HashTable of Click's Strings mapped to pointers to Scopes.
 */
typedef ScopeHashMap::iterator ScopeHashMapIter;
/** @brief A Click's HashTable of Click's Strings mapped to an InformationItem.
 */
typedef HashTable<String, InformationItem *> IIHashMap;
/** @brief An iterator to a Click's HashTable of Click's Strings mapped to an InformationItem.
 */
typedef IIHashMap::iterator IIHashMapIter;
/** @brief A Click's HashTable of Click's Strings mapped to a RemoteHost.
 */
typedef HashTable<String, RemoteHost *> RemoteHostHashMap;
/** @brief An iterator to a Click's HashTable of Click's Strings mapped to a RemoteHost.
 */
typedef RemoteHostHashMap::iterator RemoteHostHashMapIter;
/** @brief A Click's Pair of remotehosts.
 */
typedef Pair<RemoteHostSet, RemoteHostSet> RemoteHostPair;
/** @brief A Click's HashTable of Click's Strings mapped to Pair of set of RemoteHosts.
 */
typedef HashTable<String, RemoteHostPair *> IdsHashMap;
/** @brief A Click's HashTable of Click's Strings mapped to Pair of set of RemoteHosts.
 */
typedef HashTable<String, RemoteHostPair *> IdsHashMap;
/** @brief An iterator to a Click's HashTable of Click's Strings mapped to Pair of set of RemoteHosts.
 */
typedef IdsHashMap::iterator IdsHashMapIter;

class RendezvousInterface {
public:
    virtual ~RendezvousInterface();
    virtual unsigned int handleRVRequest(unsigned char type, String &nodeID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    RV *rv_element;
private:
    virtual unsigned int publish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int publish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int unpublish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int unpublish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int subscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int subscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int unsubscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
    virtual unsigned int unsubscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) = 0;
};

CLICK_ENDDECLS
#endif

