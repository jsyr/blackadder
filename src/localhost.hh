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

#ifndef CLICK_LOCALHOST_HH
#define CLICK_LOCALHOST_HH

#include <click/config.h>
#include <click/string.hh>
#include <click/hashtable.hh>

#include "common.hh"

CLICK_DECLS

/** @brief A set (implemented as a Click's HashTable) of Click's Strings.
 */
typedef HashTable<ObjectSetItem<String> > StringSet;
/** @brief An iterator to a set (implemented as a Click's HashTable) of Click's Strings.
 */
typedef StringSet::iterator StringSetIter;

/**
 * @brief (Blackadder Core) The LocalHost class represents a local software entity that accesses Blackadder's world using the provided service model.
 * 
 * Such entity can be a user-space application (or a kernel module) as well as other Click Elements (for instance the rendezvous element).
 * @param type the type of the software entity. LOCAL_PROCESS for an application or a kernel module, CLICK_MODULE for a click element.
 * @param id a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
 * @return 
 */
class LocalHost {
public:
    /** @brief Constructor:
     * @param type the type of the software entity. LOCAL_PROCESS for an application or a kernel module, CLICK_MODULE for a click element.
     * @param id a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
     */
    LocalHost(int id);
    /** @brief A Click's String uniquely identifying the LocalHost, by adding an "app", or "click" to the ineteger id. 
     */
    String localHostID;
    /**@brief a local identifier of the software entity. It can be the netlink port at which an application or a kerbel module expects data, or the out Click port to the Click Element.
     */
    unsigned int id;
    /** @brief A set of String Items identifying ActivePublications. The LocalProcy uses this set.
     */
    StringSet activePublications;
    /** @brief A set of String Items identifying ActiveSubscriptions. The LocalProcy uses this set.
     * 
     */
    StringSet activeSubscriptions;
};

CLICK_ENDDECLS

#endif
