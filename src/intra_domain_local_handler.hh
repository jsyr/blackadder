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

#ifndef CLICK_INTRADOMAINLOCALHANDLER_HH
#define CLICK_INTRADOMAINLOCALHANDLER_HH

#include <click/config.h>

#include "local_handler_interface.hh"

CLICK_DECLS

class IntraDomainLocalHandler : public LocalHandlerInterface {
public:
    IntraDomainLocalHandler(Dispatcher *_dispatcher_element);
    ~IntraDomainLocalHandler();
    void handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char &type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len);
    void handleLocalPublication(Packet *p, unsigned int local_identifier, String &ID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len);
    void handleNetworkPublication(Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/);
    void handleRVNotification(Vector<String> IDs, unsigned char strategy, unsigned int str_opt_len, const void *str_opt, Packet *p);
    void handleLocalDisconnection(unsigned int local_identifier);
    ActivePubIdx *getActivePublicationIndex();
    ActiveSubIdx *getActiveSubscriptionIndex();
    PubSubIdx *getLocalPubSubIndex();
private:
    bool storeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, bool isScope);
    bool removeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/);
    bool storeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, bool isScope);
    bool removeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/);
    void publishReqToRV(Packet *p);
    void publishReqToRV(unsigned char type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len);
    void publishDataToNetwork(Vector<String> &IDs, Packet *p /*only data*/, unsigned char strategy, const void *forwarding_information, unsigned int forwarding_information_length);
    void getFatherScopeSubscribers(String &ID, LocalHostSet &local_subscribers_to_notify);
    void deleteAllActiveInformationItemPublications(LocalHost * _publisher);
    void deleteAllActiveInformationItemSubscriptions(LocalHost * _subscriber);
    void deleteAllActiveScopePublications(LocalHost * _publisher);
    void deleteAllActiveScopeSubscriptions(LocalHost * _subscriber);
    /**@brief A HashTable that maps Publishers' and Subscribers' identifiers to pointers of LocalHost.
     * 
     * A publisher or subscriber can be an application or a Click element. Check LocalHost documentation.
     */
    PubSubIdx local_pub_sub_Index;
    /**@brief A HashTable that maps an ActivePublication identifier (full ID from a root of a graph) to a pointer of ActivePublication.
     */
    ActivePubIdx activePublicationIndex;
    /**@brief A HashTable that maps an ActiveSubscription identifier (full ID from a root of a graph) to a pointer of ActiveSubscription.
     */
    ActiveSubIdx activeSubscriptionIndex;
};

CLICK_ENDDECLS
#endif

