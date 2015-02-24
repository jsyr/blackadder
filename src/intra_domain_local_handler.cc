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
#include "intra_domain_local_handler.hh"

CLICK_DECLS
IntraDomainLocalHandler::IntraDomainLocalHandler(Dispatcher *_dispatcher_element) {
    dispatcher_element = _dispatcher_element;
}

IntraDomainLocalHandler::~IntraDomainLocalHandler() {
    int size = 0;
    size = local_pub_sub_Index.size();
    PubSubIdxIter it1 = local_pub_sub_Index.begin();
    for (int i = 0; i < size; i++) {
        delete (*it1).second;
        it1 = local_pub_sub_Index.erase(it1);
    }
    size = activePublicationIndex.size();
    ActivePubIter it2 = activePublicationIndex.begin();
    for (int i = 0; i < size; i++) {
        delete (*it2).second;
        it2 = activePublicationIndex.erase(it2);
    }
    size = activeSubscriptionIndex.size();
    ActiveSubIter it3 = activeSubscriptionIndex.begin();
    for (int i = 0; i < size; i++) {
        delete (*it3).second;
        it3 = activeSubscriptionIndex.erase(it3);
    }
}

void IntraDomainLocalHandler::handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char &type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    String fullID;
    bool forward = false;
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
            click_chatter("IntraDomainLocalHandler: received PUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = storeActivePublication(_localhost, fullID, strategy, str_opt, str_opt_len, true);
            break;
        case PUBLISH_INFO:
            click_chatter("IntraDomainLocalHandler: received PUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = storeActivePublication(_localhost, fullID, strategy, str_opt, str_opt_len, false);
            break;
        case UNPUBLISH_SCOPE:
            click_chatter("IntraDomainLocalHandler: received UNPUBLISH_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = removeActivePublication(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        case UNPUBLISH_INFO:
            click_chatter("IntraDomainLocalHandler: received UNPUBLISH_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = removeActivePublication(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        case SUBSCRIBE_SCOPE:
            click_chatter("IntraDomainLocalHandler: received SUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = storeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len, true);
            break;
        case SUBSCRIBE_INFO:
            click_chatter("IntraDomainLocalHandler: received SUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = storeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len, false);
            break;
        case UNSUBSCRIBE_SCOPE:
            click_chatter("IntraDomainLocalHandler: received UNSUBSCRIBE_SCOPE request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = removeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        case UNSUBSCRIBE_INFO:
            click_chatter("IntraDomainLocalHandler: received UNSUBSCRIBE_INFO request: %s, %s, %s, %d", _localhost->localHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            forward = removeActiveSubscription(_localhost, fullID, strategy, str_opt, str_opt_len);
            break;
        default:
            click_chatter("IntraDomainLocalHandler: unknown request - skipping request - killing packet and moving on");
            p->kill();
            return;
    }
    if (forward) {
        publishReqToRV(p);
    } else {
        p->kill();
    }
}

void IntraDomainLocalHandler::handleLocalPublication(Packet *p, unsigned int local_identifier, String &ID, unsigned char /*strategy*/, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    ActivePublication *ap;
    LocalHost *_localhost = getLocalHost(local_identifier, local_pub_sub_Index);
    ap = activePublicationIndex.get(ID);
    if (ap != activePublicationIndex.default_value()) {
        if (ap->publishers.get(_localhost) != ap->publishers.default_value()) {
            if (ap->subsFID != NULL) {
                if (ap->publishers.get(_localhost) != ap->publishers.default_value()) {
                    /*if there are local subscribers, the forward should bounce back the data as a network publication (i.e. ap->subsFID contains the iLID)*/
                    publishDataToNetwork(ap->allKnownIDs, p, ap->strategy, ap->subsFID->_data, FID_LEN);
                } else {
                    click_chatter("IntraDomainLocalHandler: publisher %d is not a publisher for item ID %s. killing the packet...", _localhost->id, ID.quoted_hex().c_str());
                    p->kill();
                }
            } else {
                click_chatter("IntraDomainLocalHandler: no rendezvous has previously taken place for item ID %s. killing the packet...", ID.quoted_hex().c_str());
                p->kill();
            }
        } else {
            click_chatter("IntraDomainLocalHandler: publisher %d is not a publisher for item ID %s. killing the packet...", _localhost->id, ID.quoted_hex().c_str());
            p->kill();
        }
    } else {
        click_chatter("IntraDomainLocalHandler: cannot publish data for ID %s locally. there is nothing advertised before. killing the packet...", ID.quoted_hex().c_str());
        p->kill();
    }
}

void IntraDomainLocalHandler::handleNetworkPublication(Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/) {
    LocalHostStringHashMap localSubscribers;
    //click_chatter("IntraDomainLocalHandler: Received data for ID: %s", IDs[0].quoted_hex().c_str());
    bool foundLocalSubscribers = findLocalSubscribers(IDs, activeSubscriptionIndex, localSubscribers);
    if (foundLocalSubscribers) {
        publishDataLocally(localSubscribers, p);
    } else {
        p->kill();
    }
}

void IntraDomainLocalHandler::handleRVNotification(Vector<String> IDs, unsigned char /*strategy*/, unsigned int /*str_opt_len*/, const void */*str_opt*/, Packet *p/*p now has only the type and any extra data with the notification (e.g. FID with START_PUBLISH)*/) {
    unsigned char type;
    ActivePublication *ap;
    bool shouldBreak = false;
    BABitvector *FID;
    type = *(p->data());
    switch (type) {
        case SCOPE_PUBLISHED:
            click_chatter("IntraDomainLocalHandler: Received notification about new scope");
            /*Find the applications to forward the notification*/
            for (int i = 0; i < IDs.size(); i++) {
                click_chatter("%s", IDs[i].quoted_hex().c_str());
                /*check the active scope subscriptions for that*/
                /*prefix-match checking here*/
                /*I should create a set of local subscribers to notify*/
                LocalHostSet local_subscribers_to_notify;
                getFatherScopeSubscribers(IDs[i], local_subscribers_to_notify);
                for (LocalHostSetIter set_it = local_subscribers_to_notify.begin(); set_it != local_subscribers_to_notify.end(); set_it++) {
                    click_chatter("IntraDomainLocalHandler: notifying Subscriber: %s", (*set_it).pointer->localHostID.c_str());
                    dispatcher_element->pushPubSubEvent((*set_it).pointer->id, SCOPE_PUBLISHED, IDs[i]);
                }
            }
            break;
        case SCOPE_UNPUBLISHED:
            click_chatter("IntraDomainLocalHandler: Received notification about a deleted scope");
            /*Find the applications to forward the notification*/
            for (int i = 0; i < IDs.size(); i++) {
                click_chatter("%s", IDs[i].quoted_hex().c_str());
                /*check the active scope subscriptions for that*/
                /*prefix-match checking here*/
                /*I should create a set of local subscribers to notify*/
                LocalHostSet local_subscribers_to_notify;
                getFatherScopeSubscribers(IDs[i], local_subscribers_to_notify);
                for (LocalHostSetIter set_it = local_subscribers_to_notify.begin(); set_it != local_subscribers_to_notify.end(); set_it++) {
                    click_chatter("IntraDomainLocalHandler: notifying Subscriber: %s", (*set_it).pointer->localHostID.c_str());
                    /*send the message*/
                    dispatcher_element->pushPubSubEvent((*set_it).pointer->id, SCOPE_UNPUBLISHED, IDs[i]);
                }
            }
            break;
        case START_PUBLISH:
            FID = new BABitvector(FID_LEN * 8);
            memcpy(FID->_data, p->data() + sizeof (type), FID_LEN);
            click_chatter("IntraDomainLocalHandler: RECEIVED FID:%s\n", FID->to_string().c_str());
            for (int i = 0; i < IDs.size(); i++) {
                click_chatter("%s", IDs[i].quoted_hex().c_str());
                ap = activePublicationIndex.get(IDs[i]);
                if (ap != activePublicationIndex.default_value()) {
                    /*copy the IDs vector to the allKnownIDs vector of the ap*/
                    ap->allKnownIDs = IDs;
                    /*this item exists*/
                    ap->subsFID = FID;
                    ap->subsExist = true;
                    /*iterate once to see if any of the publishers for this item (which may be represented by many ids) is already notified*/
                    for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
                        if ((*publishers_it).second == START_PUBLISH) {
                            //click_chatter("/*hmmm...this publisher has been previously notified*/");
                            shouldBreak = true;
                            break;
                        }
                    }
                    if (shouldBreak) {
                        break;
                    }
                }
            }
            if (!shouldBreak) {
                //click_chatter("/*none of the publishers has been previously notified*/");
                /*notify the first you find*/
                for (int i = 0; i < IDs.size(); i++) {
                    ap = activePublicationIndex.get(IDs[i]);
                    if (ap != activePublicationIndex.default_value()) {
                        /*iterate once to see if any of the publishers for this item (which may be represented by many ids) is already notified*/
                        for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
                            (*publishers_it).second = START_PUBLISH;
                            dispatcher_element->pushPubSubEvent((*publishers_it).first->id, START_PUBLISH, IDs[i]);
                            shouldBreak = true;
                            break;
                        }
                    }
                    if (shouldBreak) {
                        break;
                    }
                }
            }
            break;
        case STOP_PUBLISH:
            click_chatter("IntraDomainLocalHandler: Received NULL FID");
            for (int i = 0; i < IDs.size(); i++) {
                click_chatter("%s", IDs[i].quoted_hex().c_str());
                ap = activePublicationIndex.get(IDs[i]);
                if (ap != activePublicationIndex.default_value()) {
                    ap->allKnownIDs = IDs;
                    /*delete FID*/
                    delete ap->subsFID;
                    ap->subsFID = NULL;
                    ap->subsExist = false;
                    /*iterate once to see if any the publishers for this item (which may be represented by many ids) is already notified*/
                    for (PublisherHashMapIter publishers_it = ap->publishers.begin(); publishers_it != ap->publishers.end(); publishers_it++) {
                        if ((*publishers_it).second == START_PUBLISH) {
                            (*publishers_it).second = STOP_PUBLISH;
                            dispatcher_element->pushPubSubEvent((*publishers_it).first->id, STOP_PUBLISH, IDs[i]);
                        }
                    }
                }
            }
            break;
        default:
            click_chatter("IntraDomainLocalHandler: didn't understand the RV notification");
            break;
    }
}

void IntraDomainLocalHandler::handleLocalDisconnection(unsigned int local_identifier) {
    LocalHost *localhost = getLocalHost(local_identifier, local_pub_sub_Index);
    /*there is a bug here...I have to rethink how to correctly delete all entries in the right sequence*/
    if (localhost != NULL) {
        /*I know whether we talk about a scope or an information item from the isScope boolean value*/
        deleteAllActiveInformationItemPublications(localhost);
        deleteAllActiveInformationItemSubscriptions(localhost);
        deleteAllActiveScopePublications(localhost);
        deleteAllActiveScopeSubscriptions(localhost);
        local_pub_sub_Index.erase(localhost->id);
        delete localhost;
    }
}

void IntraDomainLocalHandler::publishReqToRV(Packet *p) {
    Vector<String> IDs;
    IDs.push_back(dispatcher_element->nodeRVIID);
    dispatcher_element->publishToNetwork(dispatcher_element->defaultRV_dl._data, FID_LEN, IDs, IMPLICIT_RENDEZVOUS, p);
}

void IntraDomainLocalHandler::publishDataToNetwork(Vector<String> &IDs, Packet *p /*only data*/, unsigned char strategy, const void *forwarding_information, unsigned int forwarding_information_length) {
    dispatcher_element->publishToNetwork(forwarding_information, forwarding_information_length, IDs, strategy, p);
}

void IntraDomainLocalHandler::getFatherScopeSubscribers(String &ID, LocalHostSet &_local_subscribers) {
    ActiveSubscription *as;
    LocalHostSetIter set_it;
    as = activeSubscriptionIndex.get(ID.substring(0, ID.length() - PURSUIT_ID_LEN));
    if (as != activeSubscriptionIndex.default_value()) {
        if (as->isScope) {
            for (set_it = as->subscribers.begin(); set_it != as->subscribers.end(); set_it++) {
                _local_subscribers.find_insert(*set_it);
            }
        }
    }
}

/*store the remote scope for the _publisher..forward the message to the RV point only if this is the first time the scope is published.
 If not, the RV point already knows about this node's publication...Note that RV points know only about network nodes - NOT for processes or click modules*/
bool IntraDomainLocalHandler::storeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, bool isScope) {
    ActivePublication *ap;
    ap = activePublicationIndex.get(fullID);
    if (ap == activePublicationIndex.default_value()) {
        /*create the active scope's publication entry*/
        ap = new ActivePublication(fullID, strategy, str_opt, str_opt_len, isScope);
        /*add the active scope's publication to the index*/
        activePublicationIndex.set(fullID, ap);
        /*update the local publishers of that active scope's publication*/
        ap->publishers.find_insert(_publisher, STOP_PUBLISH);
        /*update the active scope publications for this publsher*/
        _publisher->activePublications.find_insert(fullID);
        //click_chatter("IntraDomainLocalHandler: store Active Scope Publication %s for local publisher %s", fullID.quoted_hex().c_str(), _publisher->publisherID.c_str());
        return true;
    } else {
        if (ap->strategy == strategy) {
            /*update the publishers of that remote scope*/
            ap->publishers.find_insert(_publisher, STOP_PUBLISH);
            /*update the published remote scopes for this publsher*/
            _publisher->activePublications.find_insert(fullID);
            //click_chatter("IntraDomainLocalHandler: Active Scope Publication %s exists...updated for local publisher %s", fullID.quoted_hex().c_str(), _publisher->publisherID.c_str());
        } else {
            click_chatter("IntraDomainLocalHandler: LocalRV: error while trying to update list of publishers for active publication %s..strategy mismatch", ap->fullID.quoted_hex().c_str());
        }
    }
    return false;
}

/*delete the remote publication for the _publisher..forward the message to the RV point only if there aren't any other publishers or subscribers for this scope*/
bool IntraDomainLocalHandler::removeActivePublication(LocalHost *_publisher, String &fullID, unsigned char strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    ActivePublication *ap;
    ap = activePublicationIndex.get(fullID);
    if (ap != activePublicationIndex.default_value()) {
        if (ap->strategy == strategy) {
            _publisher->activePublications.erase(fullID);
            ap->publishers.erase(_publisher);
            //click_chatter("IntraDomainLocalHandler: deleted publisher %s from Active Scope Publication %s", _publisher->publisherID.c_str(), fullID.quoted_hex().c_str());
            if (ap->publishers.size() == 0) {
                //click_chatter("IntraDomainLocalHandler: delete Active Scope Publication %s", fullID.quoted_hex().c_str());
                delete ap;
                activePublicationIndex.erase(fullID);
                return true;
            }
        } else {
            click_chatter("IntraDomainLocalHandler: error while trying to delete active publication %s...strategy mismatch", ap->fullID.quoted_hex().c_str());
        }
    } else {
        click_chatter("IntraDomainLocalHandler:%s is not an active publication", fullID.quoted_hex().c_str());
    }
    return false;
}

/*store the active scope for the _subscriber..forward the message to the RV point only if this is the first subscription for this scope.
 If not, the RV point already knows about this node's subscription...Note that RV points know only about network nodes - NOT about processes or click modules*/
bool IntraDomainLocalHandler::storeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, bool isScope) {
    ActiveSubscription *as;
    as = activeSubscriptionIndex.get(fullID);
    if (as == activeSubscriptionIndex.default_value()) {
        as = new ActiveSubscription(fullID, strategy, str_opt, str_opt_len, isScope);
        /*add the remote scope to the index*/
        activeSubscriptionIndex.set(fullID, as);
        /*update the subscribers of that remote scope*/
        as->subscribers.find_insert(_subscriber);
        /*update the subscribed remote scopes for this publsher*/
        _subscriber->activeSubscriptions.find_insert(fullID);
        //click_chatter("IntraDomainLocalHandler: store Active Subscription %s for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
        return true;
    } else {
        if (as->strategy == strategy) {
            /*update the subscribers of that remote scope*/
            as->subscribers.find_insert(_subscriber);
            /*update the subscribed remote scopes for this publsher*/
            _subscriber->activeSubscriptions.find_insert(fullID);
            //click_chatter("IntraDomainLocalHandler: Active Subscription %s exists...updated for local subscriber %s", fullID.quoted_hex().c_str(), _subscriber->localHostID.c_str());
        } else {
            click_chatter("IntraDomainLocalHandler: error while trying to update list of subscribers for Active Subscription %s..strategy mismatch", as->fullID.quoted_hex().c_str());
        }
    }
    return false;
}

/*delete the remote scope for the _subscriber..forward the message to the RV point only if there aren't any other publishers or subscribers for this scope*/
bool IntraDomainLocalHandler::removeActiveSubscription(LocalHost *_subscriber, String &fullID, unsigned char strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    ActiveSubscription *as;
    as = activeSubscriptionIndex.get(fullID);
    if (as != activeSubscriptionIndex.default_value()) {
        if (as->strategy == strategy) {
            _subscriber->activeSubscriptions.erase(fullID);
            as->subscribers.erase(_subscriber);
            //click_chatter("IntraDomainLocalHandler: deleted subscriber %s from Active Subscription %s", _subscriber->localHostID.c_str(), fullID.quoted_hex().c_str());
            if (as->subscribers.size() == 0) {
                //click_chatter("IntraDomainLocalHandler: delete Active Subscription %s", fullID.quoted_hex().c_str());
                delete as;
                activeSubscriptionIndex.erase(fullID);
                return true;
            }
        } else {
            click_chatter("IntraDomainLocalHandler: error while trying to delete Active Subscription %s...strategy mismatch", as->fullID.quoted_hex().c_str());
        }
    } else {
        click_chatter("IntraDomainLocalHandler: no active subscriptions %s", fullID.quoted_hex().c_str());
    }
    return false;
}

void IntraDomainLocalHandler::deleteAllActiveInformationItemPublications(LocalHost * _publisher) {
    String ID, prefixID;
    bool shouldNotify = false;
    int size = _publisher->activePublications.size();
    StringSetIter it = _publisher->activePublications.begin();
    for (int i = 0; i < size; i++) {
        shouldNotify = false;
        ActivePublication *ap = activePublicationIndex.get((*it).object);
        if (!ap->isScope) {
            it = _publisher->activePublications.erase(it);
            if (ap->publishers.get(_publisher) != STOP_PUBLISH) {
                shouldNotify = true;
            }
            ap->publishers.erase(_publisher);
            click_chatter("IntraDomainLocalHandler: deleted publisher %s from Active Information Item Publication %s", _publisher->localHostID.c_str(), ap->fullID.quoted_hex().c_str());
            if (ap->publishers.size() == 0) {
                click_chatter("IntraDomainLocalHandler: delete Active Information item Publication %s", ap->fullID.quoted_hex().c_str());
                activePublicationIndex.erase(ap->fullID);
                ID = ap->fullID.substring(ap->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                prefixID = ap->fullID.substring(0, ap->fullID.length() - PURSUIT_ID_LEN);
                publishReqToRV(UNPUBLISH_INFO, ID, prefixID, ap->strategy, ap->str_opt, ap->str_opt_len);
                delete ap;
            } else {
                /*there are other local publishers...check the state of the deleted local publisher and potentially notify one of the other local publishers*/
                if (shouldNotify) {
                    /*None of the available local publishers has been previously notified*/
                    (*ap->publishers.begin()).second = START_PUBLISH;
                    dispatcher_element->pushPubSubEvent((*ap->publishers.begin()).first->id, START_PUBLISH, ap->fullID);
                }
            }
        } else {
            it++;
        }
    }
}

void IntraDomainLocalHandler::deleteAllActiveInformationItemSubscriptions(LocalHost * _subscriber) {
    String ID, prefixID;
    int size = _subscriber->activeSubscriptions.size();
    StringSetIter it = _subscriber->activeSubscriptions.begin();
    for (int i = 0; i < size; i++) {
        ActiveSubscription *as = activeSubscriptionIndex.get((*it).object);
        if (!as->isScope) {
            it = _subscriber->activeSubscriptions.erase(it);
            as->subscribers.erase(_subscriber);
            click_chatter("IntraDomainLocalHandler: deleted subscriber %s from Active Information Item Publication %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
            if (as->subscribers.size() == 0) {
                click_chatter("IntraDomainLocalHandler: delete Active Information item Subscription %s", as->fullID.quoted_hex().c_str());
                activeSubscriptionIndex.erase(as->fullID);
                /*notify the RV Function - depending on strategy*/
                ID = as->fullID.substring(as->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                prefixID = as->fullID.substring(0, as->fullID.length() - PURSUIT_ID_LEN);
                publishReqToRV(UNSUBSCRIBE_INFO, ID, prefixID, as->strategy, as->str_opt, as->str_opt_len);
                delete as;
            }
        } else {
            it++;
        }
    }
}

void IntraDomainLocalHandler::deleteAllActiveScopePublications(LocalHost * _publisher) {
    int max_level = 0;
    int temp_level;
    String ID, prefixID;
    StringSetIter it;
    for (it = _publisher->activePublications.begin(); it != _publisher->activePublications.end(); it++) {
        ActivePublication *ap = activePublicationIndex.get((*it).object);

        if (ap->isScope) {
            String temp_id = (*it).object;
            temp_level = temp_id.length() / PURSUIT_ID_LEN;
            if (temp_level > max_level) {
                max_level = temp_level;
            }
        }
    }
    for (int i = max_level; i > 0; i--) {
        it = _publisher->activePublications.begin();
        int size = _publisher->activePublications.size();
        for (int j = 0; j < size; j++) {
            String fullID = (*it).object;
            it++;
            if (fullID.length() / PURSUIT_ID_LEN == i) {
                ActivePublication *ap = activePublicationIndex.get(fullID);
                if (ap->isScope) {
                    _publisher->activePublications.erase(fullID);
                    ap->publishers.erase(_publisher);
                    click_chatter("IntraDomainLocalHandler: deleted publisher %s from Active Scope Publication %s", _publisher->localHostID.c_str(), ap->fullID.quoted_hex().c_str());
                    if (ap->publishers.size() == 0) {
                        click_chatter("IntraDomainLocalHandler: delete Active Scope Publication %s", ap->fullID.quoted_hex().c_str());
                        activePublicationIndex.erase(ap->fullID);
                        /*notify the RV Function - depending on strategy*/
                        ID = ap->fullID.substring(ap->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                        prefixID = ap->fullID.substring(0, ap->fullID.length() - PURSUIT_ID_LEN);
                        publishReqToRV(UNPUBLISH_SCOPE, ID, prefixID, ap->strategy, ap->str_opt, ap->str_opt_len);
                        delete ap;
                    }
                }
            }
        }
    }
}

void IntraDomainLocalHandler::deleteAllActiveScopeSubscriptions(LocalHost * _subscriber) {
    int max_level = 0;
    int temp_level;
    String ID, prefixID;
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
                    click_chatter("IntraDomainLocalHandler: deleted subscriber %s from Active Scope Subscription %s", _subscriber->localHostID.c_str(), as->fullID.quoted_hex().c_str());
                    if (as->subscribers.size() == 0) {
                        click_chatter("IntraDomainLocalHandler: delete Active Scope Subscription %s", as->fullID.quoted_hex().c_str());
                        activeSubscriptionIndex.erase(as->fullID);
                        /*notify the RV Function - depending on strategy*/
                        ID = as->fullID.substring(as->fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                        prefixID = as->fullID.substring(0, as->fullID.length() - PURSUIT_ID_LEN);
                        publishReqToRV(UNSUBSCRIBE_SCOPE, ID, prefixID, as->strategy, as->str_opt, as->str_opt_len);
                        delete as;
                    }
                }
            }
        }
    }
}

void IntraDomainLocalHandler::publishReqToRV(unsigned char type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    WritablePacket *p;
    Vector<String> IDs;
    unsigned int payload_len;
    unsigned char IDLength = ID.length() / PURSUIT_ID_LEN;
    unsigned char prefixIDLength = prefixID.length() / PURSUIT_ID_LEN;
    IDs.push_back(dispatcher_element->nodeRVIID);
    payload_len = sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length() + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len;
    p = InClickAPI::prepare_network_publication(dispatcher_element->defaultRV_dl._data, FID_LEN, IDs, IMPLICIT_RENDEZVOUS, payload_len);
    InClickAPI::add_data(p, &type, sizeof (type));
    InClickAPI::add_data(p, &IDLength, sizeof (IDLength));
    InClickAPI::add_data(p, ID.c_str(), ID.length());
    InClickAPI::add_data(p, &prefixIDLength, sizeof (prefixIDLength));
    InClickAPI::add_data(p, prefixID.c_str(), prefixID.length());
    InClickAPI::add_data(p, &strategy, sizeof (strategy));
    InClickAPI::add_data(p, &str_opt_len, sizeof (str_opt_len));
    InClickAPI::add_data(p, str_opt, str_opt_len);
    dispatcher_element->output(1).push(p);
}

ActivePubIdx *IntraDomainLocalHandler::getActivePublicationIndex() {
    return &activePublicationIndex;
}

ActiveSubIdx *IntraDomainLocalHandler::getActiveSubscriptionIndex() {
    return &activeSubscriptionIndex;
}

PubSubIdx *IntraDomainLocalHandler::getLocalPubSubIndex() {
    return &local_pub_sub_Index;
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(IntraDomainLocalHandler)
