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
#include "link_local_handler.hh"

CLICK_DECLS
LinkLocalHandler::LinkLocalHandler(Dispatcher *_dispatcher_element) {
    dispatcher_element = _dispatcher_element;
}

LinkLocalHandler::~LinkLocalHandler() {
    int size = 0;
    size = local_pub_sub_Index.size();
    PubSubIdxIter it1 = local_pub_sub_Index.begin();
    for (int i = 0; i < size; i++) {
        delete (*it1).second;
        it1 = local_pub_sub_Index.erase(it1);
    }
    size = activeSubscriptionIndex.size();
    ActiveSubIter it3 = activeSubscriptionIndex.begin();
    for (int i = 0; i < size; i++) {
        delete (*it3).second;
        it3 = activeSubscriptionIndex.erase(it3);
    }
}

void LinkLocalHandler::handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char &type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    String fullID;
    LocalHost *_localhost = getLocalHost(local_identifier, local_pub_sub_Index);
    /*create the fullID*/
    if (ID.length() == PURSUIT_ID_LEN) {
        /*a single fragment*/
        fullID = prefixID + ID;
    } else {
        /*multiple fragments*/
        fullID = prefixID + ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
    }
    switch (type) {
        case PUBLISH_SCOPE:
            click_chatter("LinkLocalHandler: cannot process PUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            break;
        case PUBLISH_INFO:
            click_chatter("LinkLocalHandler: cannot process  PUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            break;
        case UNPUBLISH_SCOPE:
            click_chatter("LinkLocalHandler: cannot process  UNPUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            break;
        case UNPUBLISH_INFO:
            click_chatter("LinkLocalHandler: cannot process UNPUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            break;
        case SUBSCRIBE_SCOPE:
            //click_chatter("LinkLocalHandler: received SUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            storeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len, true);
            break;
        case SUBSCRIBE_INFO:
            //click_chatter("LinkLocalHandler: received SUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            storeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len, false);
            break;
        case UNSUBSCRIBE_SCOPE:
            //click_chatter("LinkLocalHandler: received UNSUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            removeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        case UNSUBSCRIBE_INFO:
            //click_chatter("LinkLocalHandler: received UNSUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            removeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        default:
            click_chatter("LinkLocalHandler: unknown request - skipping request - killing packet and moving on");
            break;
    }
    p->kill();
}

void LinkLocalHandler::handleLocalPublication(Packet *p, unsigned int /*local_identifier*/, String &ID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    Vector<String> IDs;
    IDs.push_back(ID);
    publishDataToNetwork(IDs, p, strategy, str_opt, str_opt_len);
}

void LinkLocalHandler::handleNetworkPublication(Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/) {
    LocalHostStringHashMap localSubscribers;
    //click_chatter("LinkLocalHandler: Received data for ID: %s", IDs[0].quoted_hex().c_str());
    bool foundLocalSubscribers = findLocalSubscribers(IDs, activeSubscriptionIndex, localSubscribers);
    if (foundLocalSubscribers) {
        publishDataLocally(localSubscribers, p);
    } else {
        p->kill();
    }
}

void LinkLocalHandler::handleRVNotification(Vector<String> /*IDs*/, unsigned char /*strategy*/, unsigned int /*str_opt_len*/, const void */*str_opt*/, Packet */*p*/) {
    click_chatter("LinkLocalHandler: cannot process RV Notification");
}

void LinkLocalHandler::handleLocalDisconnection(unsigned int local_identifier) {
    LocalHost *localhost = getLocalHost(local_identifier, local_pub_sub_Index);
    /*there is a bug here...I have to rethink how to correctly delete all entries in the right sequence*/
    if (localhost != NULL) {
        deleteAllActiveInformationItemSubscriptions(localhost);
        deleteAllActiveScopeSubscriptions(localhost);
        local_pub_sub_Index.erase(localhost->id);
        delete localhost;
    }
}

void LinkLocalHandler::publishDataToNetwork(Vector<String> &IDs, Packet *p /*only data*/, unsigned char strategy, const void *forwarding_information, unsigned int forwarding_information_length) {
    if ((strategy == LINK_LOCAL) && ((forwarding_information_length != MAC_LEN) && (forwarding_information_length != IP_LEN))) {
        click_chatter("LinkLocalHandler: cannot publish data to network: LINK_LOCAL strategy needs a MAC or IP address as the strategy options");
        p->kill();
    } else if ((strategy == BROADCAST_IF) && (forwarding_information_length > 0)) {
        click_chatter("LinkLocalHandler: cannot publish data to network: BROADCAST_IF strategy must have no options (str_opt_len == 0)");
        p->kill();
    } else {
        dispatcher_element->publishToNetwork(forwarding_information, forwarding_information_length, IDs, strategy, p);
    }
}

bool LinkLocalHandler::findLocalSubscribers(Vector<String> &IDs, ActiveSubIdx &activeSubscriptionIndex, LocalHostStringHashMap & _localSubscribers) {
    bool foundSubscribers;
    String knownID;
    LocalHostSetIter set_it;
    Vector<String>::iterator id_it;
    ActiveSubscription *as;
    foundSubscribers = false;
    /*prefix-match checking here for all known IDS of aiip*/
    for (id_it = IDs.begin(); id_it != IDs.end(); id_it++) {
        knownID = *id_it;
        /*for implicit subscriptions I will look all over the structure*/
        for (int i = 0; i < knownID.length() / PURSUIT_ID_LEN; i++) {
            as = activeSubscriptionIndex.get(knownID.substring(0, knownID.length() - i * PURSUIT_ID_LEN));
            if (as != activeSubscriptionIndex.default_value()) {
                for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
                    _localSubscribers.set((*set_it).pointer, knownID);
                    foundSubscribers = true;
                }
            }
        }
    }
    return foundSubscribers;
}

/*store the active scope for the _subscriber..forward the message to the RV point only if this is the first subscription for this scope.
 If not, the RV point already knows about this node's subscription...Note that RV points know only about network nodes - NOT about processes or click modules*/
bool LinkLocalHandler::storeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, bool isScope) {
    ActiveSubscription *as;
    as = activeSubscriptionIndex.get(fullID);
    if (as == activeSubscriptionIndex.default_value()) {
        as = new ActiveSubscription(fullID, strategy, str_opt, str_opt_len, isScope);
        /*add the remote scope to the index*/
        activeSubscriptionIndex.set(fullID, as);
        /*update the subscribers of that remote scope*/
        as->subscribers.find_insert(_subscriber);
        /*update the subscribed remote scopes for this publisher*/
        _subscriber->activeSubscriptions.find_insert(fullID);
        //click_chatter("LinkLocalHandler: store Active Subscription %s for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
    } else {
        if (as->strategy == strategy) {
            /*update the subscribers of that remote scope*/
            as->subscribers.find_insert(_subscriber);
            /*update the subscribed remote scopes for this publsher*/
            _subscriber->activeSubscriptions.find_insert(fullID);
            //click_chatter("LinkLocalHandler: Active Subscription %s exists...updated for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
        } else {
            click_chatter("LinkLocalHandler: error while trying to update list of subscribers for Active Subscription %s..strategy mismatch", as->fullID.quoted_hex().c_str());
        }
    }
    return false;
}

/*delete the remote scope for the _subscriber..forward the message to the RV point only if there aren't any other publishers or subscribers for this scope*/
bool LinkLocalHandler::removeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    ActiveSubscription *as;
    as = activeSubscriptionIndex.get(fullID);
    if (as != activeSubscriptionIndex.default_value()) {
        if (as->strategy == strategy) {
            _subscriber->activeSubscriptions.erase(fullID);
            as->subscribers.erase(_subscriber);
            //click_chatter("LinkLocalHandler: deleted subscriber %s from Active Subscription %s", _subscriber->localHostID.c_str(), fullID.quoted_hex().c_str());
            if (as->subscribers.size() == 0) {
                //click_chatter("LinkLocalHandler: delete Active Subscription %s", fullID.quoted_hex().c_str());
                delete as;
                activeSubscriptionIndex.erase(fullID);
            }
        } else {
            click_chatter("LinkLocalHandler: error while trying to delete Active Subscription %s...strategy mismatch", as->fullID.quoted_hex().c_str());
        }
    } else {
        click_chatter("LinkLocalHandler: no active subscriptions %s", fullID.quoted_hex().c_str());
    }
    return false;
}

void LinkLocalHandler::deleteAllActiveInformationItemSubscriptions(LocalHost * _subscriber) {
    int size = _subscriber->activeSubscriptions.size();
    StringSetIter it = _subscriber->activeSubscriptions.begin();
    for (int i = 0; i < size; i++) {
        ActiveSubscription *as = activeSubscriptionIndex.get((*it).object);
        if (!as->isScope) {
            it = _subscriber->activeSubscriptions.erase(it);
            as->subscribers.erase(_subscriber);
            click_chatter("LinkLocalHandler: deleted subscriber %s from Active Information Item Publication %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
            if (as->subscribers.size() == 0) {
                click_chatter("LinkLocalHandler: delete Active Information item Subscription %s", as->fullID.quoted_hex().c_str());
                activeSubscriptionIndex.erase(as->fullID);
                delete as;
            }
        } else {
            it++;
        }
    }
}

void LinkLocalHandler::deleteAllActiveScopeSubscriptions(LocalHost * _subscriber) {
    int max_level = 0;
    int temp_level;
    StringSetIter it;
    for (it = _subscriber->activeSubscriptions.begin(); it != _subscriber->activeSubscriptions.end(); it++) {
        ActiveSubscription *as = activeSubscriptionIndex.get((*it).object);
        if (as->isScope) {
            String temp_id = (*it).object;
            temp_level = temp_id.length() / PURSUIT_ID_LEN;
            if (temp_level > max_level) {
                max_level = temp_level;
            }
        }
    }
    for (int i = max_level; i > 0; i--) {
        it = _subscriber->activeSubscriptions.begin();
        int size = _subscriber->activeSubscriptions.size();
        for (int j = 0; j < size; j++) {
            String fullID = (*it).object;
            it++;
            if (fullID.length() / PURSUIT_ID_LEN == i) {
                ActiveSubscription *as = activeSubscriptionIndex.get(fullID);
                if (as->isScope) {
                    _subscriber->activeSubscriptions.erase(fullID);
                    as->subscribers.erase(_subscriber);
                    click_chatter("LinkLocalHandler: deleted subscriber %s from Active Scope Subscription %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
                    if (as->subscribers.size() == 0) {
                        click_chatter("LinkLocalHandler: delete Active Scope Subscription %s", as->fullID.quoted_hex().c_str());
                        activeSubscriptionIndex.erase(as->fullID);
                        delete as;
                    }
                }
            }
        }
    }
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(LinkLocalHandler)
