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
#include "intra_node_rendezvous.hh"

CLICK_DECLS

IntraNodeRendezvous::IntraNodeRendezvous(RV *_rv_element) {
    /*/FFFFFFFFFFFFFFFD*/
    const char notification_scope_base[PURSUIT_ID_LEN] = {255, 255, 255, 255, 255, 255, 255, 253};
    rv_element = _rv_element;
    notificationIID = String(notification_scope_base, PURSUIT_ID_LEN) + rv_element->nodeID;
}

IntraNodeRendezvous::~IntraNodeRendezvous() {
    RemoteHostPair *pair_to_delete;
    for (RemoteHostHashMapIter it1 = pub_sub_Index.begin(); it1 != pub_sub_Index.end(); it1++) {
        if ((*it1).second != NULL) {
            delete (*it1).second;
            (*it1).second = NULL;
        }
    }
    pub_sub_Index.clear();
    for (ScopeHashMapIter it2 = scopeIndex.begin(); it2 != scopeIndex.end(); it2++) {
        click_chatter("Intra-Node Rendezvous: Deleting scope %s", (*it2).first.quoted_hex().c_str());
        if ((*it2).second->ids.size() == 1) {
            delete (*it2).second;
        } else {
            pair_to_delete = (*it2).second->ids.get((*it2).first);
            delete pair_to_delete;
            (*it2).second->ids.erase((*it2).first);
        }
    }
    scopeIndex.clear();
    for (IIHashMapIter it3 = pubIndex.begin(); it3 != pubIndex.end(); it3++) {
        click_chatter("Intra-Node Rendezvous: Deleting information item %s", (*it3).first.quoted_hex().c_str());
        if ((*it3).second->ids.size() == 1) {
            delete (*it3).second;
        } else {
            pair_to_delete = (*it3).second->ids.get((*it3).first);
            delete pair_to_delete;
            (*it3).second->ids.erase((*it3).first);
        }
    }
    pubIndex.clear();
}

unsigned int IntraNodeRendezvous::handleRVRequest(unsigned char type, String &nodeID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) {
    unsigned int result;
    switch (type) {
        case PUBLISH_SCOPE:
            //click_chatter("Intra-Node Rendezvous: received publish_scope request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = publish_scope(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case PUBLISH_INFO:
            //click_chatter("Intra-Node Rendezvous: received publish_info request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = publish_info(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case UNPUBLISH_SCOPE:
            //click_chatter("Intra-Node Rendezvous: received unpublish_scope request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = unpublish_scope(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case UNPUBLISH_INFO:
            //click_chatter("Intra-Node Rendezvous: received unpublish_info request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = unpublish_info(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case SUBSCRIBE_SCOPE:
            //click_chatter("Intra-Node Rendezvous: received subscribe_scope request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = subscribe_scope(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case SUBSCRIBE_INFO:
            //click_chatter("Intra-Node Rendezvous: received subscribe_info request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = subscribe_info(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case UNSUBSCRIBE_SCOPE:
            //click_chatter("Intra-Node Rendezvous: received unsubscribe_scope request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = unsubscribe_scope(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case UNSUBSCRIBE_INFO:
            //click_chatter("Intra-Node Rendezvous: received unsubscribe_info request: %s, %s, %s, %d", _remotehost->remoteHostID.c_str(), ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str(), (int) strategy);
            result = unsubscribe_info(nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        default:
            click_chatter("Intra-Node Rendezvous: unknown request type - skipping request");
            result = UNKNOWN_REQUEST_TYPE;
            break;
    }
    return result;
}

unsigned int IntraNodeRendezvous::publish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) {
    unsigned int ret;
    /*When a Scope is published the RV point (that is this node) should notify interested subscribers about the new scope*/
    /*For each subscriber the RV point should use the appropriate ID path*/
    if ((prefixID.length() == 0) && (ID.length() == PURSUIT_ID_LEN)) {
        ret = publish_root_scope(publisherID, ID, strategy, str_opt, str_opt_len);
    } else if ((prefixID.length() > 0) && (ID.length() == PURSUIT_ID_LEN)) {
        ret = publish_inner_scope(publisherID, ID, prefixID, strategy, str_opt, str_opt_len);
    } else if ((prefixID.length() > 0) && (ID.length() > PURSUIT_ID_LEN)) {
        ret = republish_inner_scope(publisherID, ID, prefixID, strategy, str_opt, str_opt_len);
    } else {
        ret = WRONG_IDS;
        click_chatter("Intra-Node Rendezvous: error while publishing scope. ID: %s - prefixID: %s", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
    }
    return ret;
}

unsigned int IntraNodeRendezvous::publish_root_scope(String &publisherID, String &ID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    /*when root scopes are published there is no need to notify subscribers*/
    Scope *sc = scopeIndex.get(ID);
    if (sc == scopeIndex.default_value()) {
        sc = new Scope(strategy, NULL);
        scopeIndex.set(ID, sc);
        if (sc->updatePublishers(ID, _publisher)) {
            /*add the scope to the publisher's set*/
            _publisher->publishedScopes.find_insert(ID);
            click_chatter("Intra-Node Rendezvous: added publisher %s to (new) scope: %s(%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
            ret = SUCCESS;
        } else {
            ret = EXISTS;
        }
    } else {
        /*check if the strategies match*/
        if (sc->strategy == strategy) {
            if (sc->updatePublishers(ID, _publisher)) {
                /*add the scope to the publisher's set*/
                _publisher->publishedScopes.find_insert(ID);
                click_chatter("Intra-Node Rendezvous: added publisher %s to scope: %s(%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                ret = SUCCESS;
            } else {
                ret = EXISTS;
            }
        } else {
            click_chatter("Intra-Node Rendezvous: strategies don't match....aborting");
            ret = STRATEGY_MISMATCH;
        }
    }
    return ret;
}

unsigned int IntraNodeRendezvous::publish_inner_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    Scope *sc;
    Scope *fatherScope;
    String fullID;
    /*the publisher publishes a scope (a single fragment ID is used) under a path that must exist*/
    /*check if a InformationItem with the same path_id exists*/
    fullID = prefixID + ID;
    if (pubIndex.find(fullID) == pubIndex.end()) {
        /*check if the father scope exists*/
        fatherScope = scopeIndex.get(prefixID);
        if (fatherScope != scopeIndex.default_value()) {
            /*check if the scope under publication exists..*/
            sc = scopeIndex.get(fullID);
            if (sc == scopeIndex.default_value()) {
                /*it does not exist...create a new scope*/
                /*check the strategy of the father scope*/
                if (fatherScope->strategy == strategy) {
                    sc = new Scope(strategy, fatherScope);
                    sc->recursivelyUpdateIDs(scopeIndex, pubIndex, fullID.substring(fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN));
                    if (sc->updatePublishers(fullID, _publisher)) {
                        /*add the scope to the publisher's set*/
                        _publisher->publishedScopes.find_insert(fullID);
                        click_chatter("Intra-Node Rendezvous: added publisher %s to (new) scope: %s(%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        /*notify subscribers here!!*/
                        /*differently compared to the republish_inner_scope case*/
                        RemoteHostSet subscribers;
                        StringSet _ids;
                        fatherScope->getSubscribers(subscribers);
                        sc->getIDs(_ids);
                        notifySubscribers(SCOPE_PUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: error while publishing scope - father scope %s has incompatible strategy...", fatherScope->printID().c_str());
                    ret = STRATEGY_MISMATCH;
                }
            } else {
                if (sc->strategy == strategy) {
                    if (sc->updatePublishers(fullID, _publisher)) {
                        /*add the scope to the publisher's set*/
                        _publisher->publishedScopes.find_insert(fullID);
                        /*DO NOT notify subscribers - they already know about that scope!!*/
                        click_chatter("Intra-Node Rendezvous: added publisher %s to scope: %s(%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: scope %s exists..but with a different strategy", sc->printID().c_str());
                    ret = STRATEGY_MISMATCH;
                }
            }
        } else {
            click_chatter("Intra-Node Rendezvous: error while publishing scope %s under %s which does not exist!", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
            ret = FATHER_DOES_NOT_EXIST;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: error - a piece of info with the same path_id exists");
        ret = INFO_ITEM_WITH_SAME_ID;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::republish_inner_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    Scope *equivalentScope;
    Scope *existingScope;
    Scope *fatherScope;
    /*The publisher republishes an inner scope under an existing scope*/
    String suffixID = ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
    String fullID = prefixID + suffixID;
    String existingPrefixID = ID.substring(0, ID.length() - PURSUIT_ID_LEN);
    if (pubIndex.find(fullID) == pubIndex.end()) {
        /*the publisher publishes an existing scope under a path that must exist*/
        fatherScope = scopeIndex.get(prefixID);
        if (fatherScope != scopeIndex.default_value()) {
            existingScope = scopeIndex.get(ID);
            if (existingScope != scopeIndex.default_value()) {
                equivalentScope = scopeIndex.get(fullID);
                if (equivalentScope == scopeIndex.default_value()) {
                    if (fatherScope->strategy == strategy) {
                        existingScope->fatherScopes.find_insert(fatherScope);
                        fatherScope->childrenScopes.find_insert(existingScope);
                        existingScope->recursivelyUpdateIDs(scopeIndex, pubIndex, suffixID);
                        if (existingScope->updatePublishers(fullID, _publisher)) {
                            /*add the scope to the publisher's set*/
                            _publisher->publishedScopes.find_insert(fullID);
                            click_chatter("Intra-Node Rendezvous: added publisher %s to republished scope%s under scope %s(%d)", _publisher->remoteHostID.c_str(), existingScope->printID().c_str(), fatherScope->printID().c_str(), (int) strategy);
                            /*notify subscribers here - careful to use the right father as a start!!*/
                            RemoteHostSet subscribers;
                            StringSet _ids;
                            fatherScope->getSubscribers(subscribers);
                            existingScope->getIDs(_ids);
                            _ids.erase(existingPrefixID + suffixID);
                            notifySubscribers(SCOPE_PUBLISHED, _ids, existingScope->strategy, existingScope->str_opt, existingScope->str_opt_len, subscribers);
                            ret = SUCCESS;
                        } else {
                            ret = EXISTS;
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: error while republishing father scope %s has incompatible strategy...", fatherScope->printID().c_str());
                        ret = STRATEGY_MISMATCH;
                    }
                } else {
                    if (equivalentScope->strategy == strategy) {
                        if (equivalentScope->updatePublishers(fullID, _publisher)) {
                            /*add the scope to the publisher's set*/
                            _publisher->publishedScopes.find_insert(fullID);
                            /*DO NOT notify subscribers - they already know about that scope (the republication)!!*/
                            click_chatter("Intra-Node Rendezvous: added publisher %s to scope: %s(%d)", _publisher->remoteHostID.c_str(), equivalentScope->printID().c_str(), (int) strategy);
                            ret = SUCCESS;
                        } else {
                            ret = EXISTS;
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: scope %s exists..but with a different strategy", equivalentScope->printID().c_str());
                        ret = STRATEGY_MISMATCH;
                    }
                }
            } else {
                click_chatter("Intra-Node Rendezvous: error - cannot (re)publish scope %s somewhere else because it doesn't exist", ID.quoted_hex().c_str());
                ret = SCOPE_DOES_NOT_EXIST;
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Error - cannot (re)publish scope %s under %s which does not exist!", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
            ret = FATHER_DOES_NOT_EXIST;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: Error - A piece of info with the same ID exists");
        ret = INFO_ITEM_WITH_SAME_ID;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::publish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) {
    unsigned int ret;
    if ((prefixID.length() > 0) && (ID.length() == PURSUIT_ID_LEN)) {
        ret = advertise_info(publisherID, ID, prefixID, strategy, str_opt, str_opt_len);
    } else if ((prefixID.length() > 0) && (ID.length() > PURSUIT_ID_LEN)) {
        ret = readvertise_info(publisherID, ID, prefixID, strategy, str_opt, str_opt_len);
    } else {
        ret = WRONG_IDS;
        click_chatter("Intra-Node Rendezvous: error while publishing information. ID: %s - prefixID: %s", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
    }
    return ret;
}

unsigned int IntraNodeRendezvous::advertise_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHostSet _publishers;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    InformationItem *pub;
    Scope *fatherScope;
    String fullID = prefixID + ID;
    /*the publisher advertises a piece (a single ID fragment) of info under a path that must exist*/
    fatherScope = scopeIndex.get(prefixID);
    if (fatherScope != scopeIndex.default_value()) {
        /*check if the InformationItem (with this specific ID) is already there...*/
        pub = pubIndex.get(fullID);
        if (pub == pubIndex.default_value()) {
            /*check if a scope with the same ID exists*/
            if (scopeIndex.find(fullID) == scopeIndex.end()) {
                if (fatherScope->strategy == strategy) {
                    pub = new InformationItem(fatherScope->strategy, fatherScope);
                    pub->updateIDs(pubIndex, fullID.substring(fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN));
                    if (pub->updatePublishers(fullID, _publisher)) {
                        /*add the InformationItem to the publisher's set*/
                        _publisher->publishedInformationItems.find_insert(fullID);
                        click_chatter("Intra-Node Rendezvous: added publisher %s to (new) InformationItem: %s(%d)", _publisher->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                        RemoteHostSet subscribers;
                        pub->getSubscribers(subscribers);
                        fatherScope->getSubscribers(subscribers);
                        pub->getPublishers(_publishers);
                        rendezvous(pub, _publishers, subscribers);
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: Error could not add InformationItem - strategy mismatch");
                    ret = STRATEGY_MISMATCH;
                }
            } else {
                click_chatter("Intra-Node Rendezvous: Error - a scope with the same ID exists");
                ret = SCOPE_WITH_SAME_ID;
            }
        } else {
            if (fatherScope->strategy == strategy) {
                if (pub->updatePublishers(fullID, _publisher)) {
                    /*add the InformationItem to the publisher's set*/
                    _publisher->publishedInformationItems.find_insert(fullID);
                    click_chatter("Intra-Node Rendezvous: added publisher %s to InformationItem: %s(%d)", _publisher->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                    RemoteHostSet subscribers;
                    pub->getSubscribers(subscribers);
                    /*careful here...this pub MAY have multiple fathers*/
                    for (ScopeSetIter fathersc_it = pub->fatherScopes.begin(); fathersc_it != pub->fatherScopes.end(); fathersc_it++) {
                        (*fathersc_it).pointer->getSubscribers(subscribers);
                    }
                    pub->getPublishers(_publishers);
                    rendezvous(pub, _publishers, subscribers);
                    ret = SUCCESS;
                } else {
                    ret = EXISTS;
                }
            } else {
                click_chatter("Intra-Node Rendezvous: Error could not update InformationItem - strategy mismatch");
                ret = STRATEGY_MISMATCH;
            }
        }
    } else {
        click_chatter("Intra-Node Rendezvous: Error - Scope prefix %s doesn't exist", prefixID.quoted_hex().c_str());
        ret = FATHER_DOES_NOT_EXIST;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::readvertise_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    RemoteHost *_publisher = getRemoteHost(publisherID);
    RemoteHostSet _publishers;
    unsigned int ret;
    InformationItem *existingPub;
    InformationItem *equivalentPub;
    Scope *fatherScope;
    String suffixID = ID.substring(ID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
    String fullID = prefixID + suffixID;
    /*the publisher re-advertises an existing InformationItem under a path that must exist*/
    /*in this case the InformationItem will have more than one IDs*/
    /*check if a scope with the same ID exists*/
    if (scopeIndex.find(fullID) == scopeIndex.end()) {
        fatherScope = scopeIndex.get(prefixID);
        /*check if the parent scope exists*/
        if (fatherScope != scopeIndex.default_value()) {
            /*check if the InformationItem exists..*/
            existingPub = pubIndex.get(ID);
            if (existingPub != pubIndex.default_value()) {
                /*check if the InformationItem under the new path exists*/
                equivalentPub = pubIndex.get(fullID);
                if (equivalentPub == pubIndex.default_value()) {
                    if (fatherScope->strategy == strategy) {
                        existingPub->fatherScopes.find_insert(fatherScope);
                        fatherScope->informationitems.find_insert(existingPub);
                        existingPub->updateIDs(pubIndex, suffixID);
                        if (existingPub->updatePublishers(fullID, _publisher)) {
                            /*add the InformationItem to the publisher's set*/
                            _publisher->publishedInformationItems.find_insert(fullID);
                            click_chatter("Intra-Node Rendezvous: added publisher %s to readvertised InformationItem %s under path %s (%d)", _publisher->remoteHostID.c_str(), existingPub->printID().c_str(), fatherScope->printID().c_str(), (int) strategy);
                            RemoteHostSet subscribers;
                            existingPub->getSubscribers(subscribers);
                            /*careful here...I have multiple fathers*/
                            for (ScopeSetIter fathersc_it = existingPub->fatherScopes.begin(); fathersc_it != existingPub->fatherScopes.end(); fathersc_it++) {
                                (*fathersc_it).pointer->getSubscribers(subscribers);
                            }
                            existingPub->getPublishers(_publishers);
                            rendezvous(existingPub, _publishers, subscribers);
                            ret = SUCCESS;
                        } else {
                            ret = EXISTS;
                        }
                    } else {
                        ret = STRATEGY_MISMATCH;
                        click_chatter("Intra-Node Rendezvous: Error could not add InformationItem- strategy mismatch");
                    }
                } else {
                    if (fatherScope->strategy == strategy) {
                        if (equivalentPub->updatePublishers(fullID, _publisher)) {
                            /*add the InformationItem to the publisher's set*/
                            _publisher->publishedInformationItems.find_insert(fullID);
                            click_chatter("Intra-Node Rendezvous: added publisher %s to InformationItem: %s(%d)", _publisher->remoteHostID.c_str(), equivalentPub->printID().c_str(), (int) strategy);
                            RemoteHostSet subscribers;
                            equivalentPub->getSubscribers(subscribers);
                            /*careful here...I have multiple fathers*/
                            for (ScopeSetIter fathersc_it = equivalentPub->fatherScopes.begin(); fathersc_it != equivalentPub->fatherScopes.end(); fathersc_it++) {
                                (*fathersc_it).pointer->getSubscribers(subscribers);
                            }
                            equivalentPub->getPublishers(_publishers);
                            rendezvous(equivalentPub, _publishers, subscribers);
                            ret = SUCCESS;
                        } else {
                            ret = EXISTS;
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: Error could not republish InformationItem - strategy mismatch");
                        ret = STRATEGY_MISMATCH;
                    }
                }
            } else {
                click_chatter("Intra-Node Rendezvous: Error - cannot (re)advertise info %s somewhere else because it doesn't exist", ID.quoted_hex().c_str());
                ret = INFO_DOES_NOT_EXIST;
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Error - (re)advertise info under %s that doesn't exist!", prefixID.quoted_hex().c_str());
            ret = FATHER_DOES_NOT_EXIST;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: Error - a scope with the same ID exists");
        ret = SCOPE_WITH_SAME_ID;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::unpublish_scope(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) {
    unsigned int ret;
    Scope *sc;
    Scope *fatherScope;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    String fullID = prefixID + ID;
    sc = scopeIndex.get(fullID);
    if (sc != scopeIndex.default_value()) {
        if (sc->strategy == strategy) {
            fatherScope = scopeIndex.get(prefixID);
            if (fatherScope != scopeIndex.default_value()) {
                /*not a root scope*/
                /*try to unpublish all InformationItems under that scope*/
                for (InformationItemSetIter pub_it = sc->informationitems.begin(); pub_it != sc->informationitems.end(); pub_it++) {
                    InformationItem *pub = (*pub_it).pointer;
                    String pubSuffixID = (*pub->ids.begin()).first.substring((*pub->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                    /*call unpublish_info() for all IDs of this scope*/
                    for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                        String scPrefixID = (*it).first;
                        unpublish_info(publisherID, pubSuffixID, scPrefixID, strategy, str_opt, str_opt_len);
                    }
                }
                RemoteHostPair * pair = sc->ids.get(fullID);
                /*erase _publisher (if it exists) from the appropriate ID pair*/
                if (pair->first.find(_publisher) != pair->first.end()) {
                    pair->first.erase(_publisher);
                    _publisher->publishedScopes.erase(fullID);
                    /*do not try to delete if there are subscopes or InformationItems under the scope*/
                    if ((sc->childrenScopes.size() == 0) && (sc->informationitems.size() == 0)) {
                        /*different approach is followed depending on the number of father scopes (NOT on the number of IDS)*/
                        if (sc->fatherScopes.size() == 1) {
                            if (!sc->checkForOtherPubSub(fatherScope)) {
                                /*notify subscribers about deletion*/
                                RemoteHostSet subscribers;
                                StringSet _ids;
                                fatherScope->getSubscribers(subscribers);
                                sc->getIDs(_ids);
                                notifySubscribers(SCOPE_UNPUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                                /*safe to delete Scope*/
                                fatherScope->childrenScopes.erase(sc);
                                click_chatter("Intra-Node Rendezvous: deleted publisher %s from (deleted) scope %s (%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                                /*delete all IDs from scopeIndex*/
                                for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                                    scopeIndex.erase(it.key());
                                }
                                delete sc;
                            } else {
                                click_chatter("Intra-Node Rendezvous: deleted publisher %s from scope %s(%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                            }
                        } else {
                            if (!sc->checkForOtherPubSub(fatherScope)) {
                                /*notify subscribers about deletion*/
                                RemoteHostSet subscribers;
                                StringSetIter _ids_it;
                                StringSet _ids;
                                fatherScope->getSubscribers(subscribers);
                                sc->getIDs(_ids);
                                /*I have to delete identifiers of all other branches!!!!!*/
                                for (_ids_it = _ids.begin(); _ids_it != _ids.end(); _ids_it++) {
                                    if ((*_ids_it).object.compare(fullID) != 0) {
                                        _ids.erase((*_ids_it).object);
                                    }
                                }
                                notifySubscribers(SCOPE_UNPUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                                /*safe to delete scope (only this the specific branch)*/
                                fatherScope->childrenScopes.erase(sc);
                                sc->fatherScopes.erase(fatherScope);
                                click_chatter("Intra-Node Rendezvous: deleted publisher %s from (deleted) scope branch %s(%d)", _publisher->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                                /*delete all IDs from scopeIndex*/
                                String suffixID = (*sc->ids.begin()).first.substring((*sc->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                                for (IdsHashMapIter it = fatherScope->ids.begin(); it != fatherScope->ids.end(); it++) {
                                    /*since the scope is not deleted, manually delete this pair*/
                                    delete sc->ids.get((*it).first + suffixID);
                                    sc->ids.erase((*it).first + suffixID);
                                    scopeIndex.erase((*it).first + suffixID);
                                }
                            } else {
                                click_chatter("Intra-Node Rendezvous: deleted publisher %s from scope branch %s(%d)", _publisher->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                            }
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from scope %s (%d)", _publisher->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                    }
                    /*SUCCESS here means only that the publisher was removed from the scope - It does not mean that the scope was unpublished*/
                    ret = SUCCESS;
                } else {
                    ret = DOES_NOT_EXIST;
                }
            } else {
                /*a ROOT scope*/
                /*try to unpublish all InformationItems under that scope*/
                InformationItemSetIter pub_it;
                for (pub_it = sc->informationitems.begin(); pub_it != sc->informationitems.end(); pub_it++) {
                    InformationItem *pub = (*pub_it).pointer;
                    String pubSuffixID = (*pub->ids.begin()).first.substring((*pub->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                    /*call unpublish_info() for all IDs of this scope*/
                    for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                        String scPrefixID = (*it).first;
                        unpublish_info(publisherID, pubSuffixID, scPrefixID, strategy, str_opt, str_opt_len);
                    }
                }
                RemoteHostPair * pair = sc->ids.get(fullID);
                /*erase _publisher (if it exists) from the appropriate ID pair*/
                if (pair->first.find(_publisher) != pair->first.end()) {
                    pair->first.erase(_publisher);
                    _publisher->publishedScopes.erase(fullID);
                    /*do not try to delete if there are subscopes or InformationItems under the scope*/
                    if ((sc->childrenScopes.size() == 0) && (sc->informationitems.size() == 0)) {
                        if (!sc->checkForOtherPubSub(NULL)) {
                            click_chatter("Intra-Node Rendezvous: deleted publisher %s from (deleted) scope %s (%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                            /*delete all IDs from scopeIndex*/
                            for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                                scopeIndex.erase((*it).first);
                            }
                            delete sc;
                        } else {
                            click_chatter("Intra-Node Rendezvous: deleted publisher %s from scope %s (%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from scope %s (%d)", _publisher->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                    }
                    /*SUCCESS here means only that the publisher was removed from the scope - It does not mean that the scope was unpublished*/
                    ret = SUCCESS;
                } else {
                    ret = DOES_NOT_EXIST;
                }
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Cannot unpublish scope %s..strategy mismatch", fullID.quoted_hex().c_str());
            ret = STRATEGY_MISMATCH;
        }
    } else {
        ret = SCOPE_DOES_NOT_EXIST;
        click_chatter("Intra-Node Rendezvous: Scope %s does not exist...unpublish what?", fullID.quoted_hex().c_str());
    }
    return ret;
}

unsigned int IntraNodeRendezvous::unpublish_info(String &publisherID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    InformationItem *pub;
    Scope *fatherScope;
    RemoteHostSet _publishers;
    RemoteHost *_publisher = getRemoteHost(publisherID);
    String fullID = prefixID + ID;
    /*check if the publisher exists in general and get a pointer to the object*/
    pub = pubIndex.get(fullID);
    fatherScope = scopeIndex.get(prefixID);
    if (pub != pubIndex.default_value()) {
        if (fatherScope->strategy == strategy) {
            RemoteHostPair * pair = pub->ids.get(fullID);
            /*erase _publisher (if it exists) from the appropriate ID pair*/
            if (pair->first.find(_publisher) != pair->first.end()) {
                pair->first.erase(_publisher);
                _publisher->publishedInformationItems.erase(fullID);
                /*different approach is followed depending on the number of father scopes (NOT on the number of IDS)*/
                if (pub->fatherScopes.size() == 1) {
                    if (!pub->checkForOtherPubSub(fatherScope)) {
                        /*safe to delete InformationItem*/
                        fatherScope->informationitems.erase(pub);
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from (deleted) InformationItem %s (%d)", _publisher->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                        /*delete all IDs from pubIndex*/
                        for (IdsHashMapIter it = pub->ids.begin(); it != pub->ids.end(); it++) {
                            pubIndex.erase((*it).first);
                        }
                        delete pub;
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from InformationItem %s(%d)", _publisher->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                        RemoteHostSet subscribers;
                        pub->getSubscribers(subscribers);
                        /*careful here...I have multiple fathers*/
                        for (ScopeSetIter fathersc_it = pub->fatherScopes.begin(); fathersc_it != pub->fatherScopes.end(); fathersc_it++) {
                            (*fathersc_it).pointer->getSubscribers(subscribers);
                        }
                        pub->getPublishers(_publishers);
                        rendezvous(pub, _publishers, subscribers);
                    }
                } else {
                    if (!pub->checkForOtherPubSub(fatherScope)) {
                        /*safe to delete InformationItem*/
                        fatherScope->informationitems.erase(pub);
                        pub->fatherScopes.erase(fatherScope);
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from (deleted) InformationItem branch %s(%d)", _publisher->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                        /*delete all IDs from pubIndex*/
                        String suffixID = (*pub->ids.begin()).first.substring((*pub->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                        for (IdsHashMapIter it = fatherScope->ids.begin(); it != fatherScope->ids.end(); it++) {
                            /*since the pub is not deleted, manually delete this pair*/
                            delete pub->ids.get((*it).first + suffixID);
                            pub->ids.erase((*it).first + suffixID);
                            pubIndex.erase((*it).first + suffixID);
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted publisher %s from InformationItem branch %s(%d)", _publisher->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                    }
                    RemoteHostSet subscribers;
                    pub->getSubscribers(subscribers);
                    /*careful here...I have multiple fathers*/
                    for (ScopeSetIter fathersc_it = pub->fatherScopes.begin(); fathersc_it != pub->fatherScopes.end(); fathersc_it++) {
                        (*fathersc_it).pointer->getSubscribers(subscribers);
                    }
                    pub->getPublishers(_publishers);
                    rendezvous(pub, _publishers, subscribers);
                    /********************************************/
                }
                /*SUCCESS here means only that the publisher was removed from the info - It does not mean that the item was unpublished*/
                ret = SUCCESS;
            } else {
                ret = DOES_NOT_EXIST;
            }
        } else {
            click_chatter("Intra-Node Rendezvous:Cannot unpublish %s..strategy mismatch", pub->printID().c_str());
            ret = STRATEGY_MISMATCH;
        }
    } else {
        click_chatter("Intra-Node Rendezvous:InformationItem %s does not exist...unpublish what", fullID.quoted_hex().c_str());
        ret = INFO_DOES_NOT_EXIST;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::subscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void *str_opt, unsigned int str_opt_len) {
    unsigned int ret;
    if ((prefixID.length() == 0) && (ID.length() == PURSUIT_ID_LEN)) {
        ret = subscribe_root_scope(subscriberID, ID, strategy, str_opt, str_opt_len);
    } else if ((prefixID.length() > 0) && (ID.length() == PURSUIT_ID_LEN)) {
        ret = subscribe_inner_scope(subscriberID, ID, prefixID, strategy, str_opt, str_opt_len);
    } else {
        ret = WRONG_IDS;
        click_chatter("Intra-Node Rendezvous: error while subscribing to scope. ID: %s - prefixID: %s", ID.quoted_hex().c_str(), prefixID.quoted_hex().c_str());
    }
    return ret;
}

unsigned int IntraNodeRendezvous::subscribe_root_scope(String &subscriberID, String &ID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHostSet _publishers;
    RemoteHost *_subscriber = getRemoteHost(subscriberID);
    Scope *sc = NULL;
    sc = scopeIndex.get(ID);
    if (sc == scopeIndex.default_value()) {
        /*the root scope does not exist. Create it and add subscription*/
        sc = new Scope(strategy, NULL);
        scopeIndex.set(ID, sc);
        if (sc->updateSubscribers(ID, _subscriber)) {
            /*add the scope to the subscriber's set*/
            _subscriber->subscribedScopes.find_insert(ID);
            click_chatter("Intra-Node Rendezvous: added subscriber %s to (new) scope %s (%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
            ret = SUCCESS;
        } else {
            ret = EXISTS;
        }
    } else {
        /*check if the strategies match*/
        if (sc->strategy == strategy) {
            if (sc->updateSubscribers(ID, _subscriber)) {
                /*add the scope to the subscriber's set*/
                _subscriber->subscribedScopes.find_insert(ID);
                click_chatter("Intra-Node Rendezvous: added subscriber %s to scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                /*first notify the subscriber about the existing subscopes*/
                RemoteHostSet subscribers;
                ScopeSet _subscopes;
                subscribers.find_insert(_subscriber);
                sc->getSubscopes(_subscopes);
                for (ScopeSetIter sc_set_it = _subscopes.begin(); sc_set_it != _subscopes.end(); sc_set_it++) {
                    StringSet _ids;
                    (*sc_set_it).pointer->getIDs(_ids);
                    notifySubscribers(SCOPE_PUBLISHED, _ids, (*sc_set_it).pointer->strategy, (*sc_set_it).pointer->str_opt, (*sc_set_it).pointer->str_opt_len, subscribers);
                }
                /*then find all InformationItems for which the _subscriber is interested in*/
                InformationItemSet _informationitems;
                sc->getInformationItems(_informationitems);
                InformationItemSetIter pub_it;
                for (pub_it = _informationitems.begin(); pub_it != _informationitems.end(); pub_it++) {
                    RemoteHostSet subscribers;
                    (*pub_it).pointer->getSubscribers(subscribers);
                    /*careful here...I have multiple fathers*/
                    for (ScopeSetIter fathersc_it = (*pub_it).pointer->fatherScopes.begin(); fathersc_it != (*pub_it).pointer->fatherScopes.end(); fathersc_it++) {
                        (*fathersc_it).pointer->getSubscribers(subscribers);
                    }
                    (*pub_it).pointer->getPublishers(_publishers);
                    rendezvous((*pub_it).pointer, _publishers, subscribers);
                }
                ret = SUCCESS;
            } else {
                ret = EXISTS;
            }
        } else {
            click_chatter("Intra-Node Rendezvous: strategies don't match....aborting subscription");
            ret = STRATEGY_MISMATCH;
        }
    }
    return ret;
}

unsigned int IntraNodeRendezvous::subscribe_inner_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    RemoteHost *_subscriber = getRemoteHost(subscriberID);
    RemoteHostSet _publishers;
    Scope *sc;
    Scope *fatherScope;
    String fullID;
    /*the publisher publishes a scope (a single fragment ID is used) under a path that must exist*/
    /*check if a InformationItem with the same path_id exists*/
    fullID = prefixID + ID;
    if (pubIndex.find(fullID) == pubIndex.end()) {
        /*check if the father scope exists*/
        fatherScope = scopeIndex.get(prefixID);
        if (fatherScope != scopeIndex.default_value()) {
            /*check if the scope under publication exists..*/
            sc = scopeIndex.get(fullID);
            if (sc == scopeIndex.default_value()) {
                /*it does not exist...create a new scope and add subscription*/
                /*check the strategy of the father scope*/
                if (fatherScope->strategy == strategy) {
                    sc = new Scope(strategy, fatherScope);
                    sc->recursivelyUpdateIDs(scopeIndex, pubIndex, fullID.substring(fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN));
                    if (sc->updateSubscribers(fullID, _subscriber)) {
                        /*add the scope to the publisher's set*/
                        _subscriber->subscribedScopes.find_insert(fullID);
                        click_chatter("Intra-Node Rendezvous: added subscriber %s to (new) scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        /*WEIRD BUT notify other subscribers since the scope has been created!!*/
                        RemoteHostSet subscribers;
                        StringSet _ids;
                        fatherScope->getSubscribers(subscribers);
                        sc->getIDs(_ids);
                        notifySubscribers(SCOPE_PUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: error while subscribing to scope - father scope %s has incompatible strategy...", fatherScope->printID().c_str());
                    ret = STRATEGY_MISMATCH;
                }
            } else {
                if (sc->strategy == strategy) {
                    if (sc->updateSubscribers(fullID, _subscriber)) {
                        /*add the scope to the subscriber's set*/
                        _subscriber->subscribedScopes.find_insert(fullID);
                        click_chatter("Intra-Node Rendezvous: added subscriber %s to scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        /*first notify the subscriber about the existing subscopes*/
                        RemoteHostSet subscribers;
                        ScopeSet _subscopes;
                        subscribers.find_insert(_subscriber);
                        sc->getSubscopes(_subscopes);
                        for (ScopeSetIter sc_set_it = _subscopes.begin(); sc_set_it != _subscopes.end(); sc_set_it++) {
                            StringSet _ids;
                            (*sc_set_it).pointer->getIDs(_ids);
                            notifySubscribers(SCOPE_PUBLISHED, _ids, (*sc_set_it).pointer->strategy, (*sc_set_it).pointer->str_opt, (*sc_set_it).pointer->str_opt_len, subscribers);
                        }
                        /*then find all InformationItems for which the _subscriber is interested in*/
                        InformationItemSet _informationitems;
                        sc->getInformationItems(_informationitems);
                        /*then, for each one do the rendez-vous process*/
                        InformationItemSetIter pub_it;
                        for (pub_it = _informationitems.begin(); pub_it != _informationitems.end(); pub_it++) {
                            RemoteHostSet subscribers;
                            (*pub_it).pointer->getSubscribers(subscribers);
                            /*careful here...I have multiple fathers*/
                            ScopeSetIter fathersc_it;
                            for (fathersc_it = (*pub_it).pointer->fatherScopes.begin(); fathersc_it != (*pub_it).pointer->fatherScopes.end(); fathersc_it++) {
                                (*fathersc_it).pointer->getSubscribers(subscribers);
                            }
                            (*pub_it).pointer->getPublishers(_publishers);
                            rendezvous((*pub_it).pointer, _publishers, subscribers);
                        }
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    ret = STRATEGY_MISMATCH;
                    click_chatter("Intra-Node Rendezvous: strategies don't match....aborting subscription");
                }
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Cannot subscribe - father scope not exist!");
            ret = FATHER_DOES_NOT_EXIST;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: Cannot subscribe to scope - a piece of info with the same path_id exists");
        ret = INFO_ITEM_WITH_SAME_ID;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::subscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    InformationItem *pub;
    Scope *fatherScope;
    RemoteHostSet _publishers;
    RemoteHost *_subscriber = getRemoteHost(subscriberID);
    if ((prefixID.length() > 0) && (ID.length() == PURSUIT_ID_LEN)) {
        String fullID = prefixID + ID;
        /*the publisher advertises a piece (a single ID fragment) of info under a path that must exist*/
        fatherScope = scopeIndex.get(prefixID);
        if (fatherScope != scopeIndex.default_value()) {
            /*check if the InformationItem (with this specific ID) is already there...*/
            pub = pubIndex.get(fullID);
            if (pub == pubIndex.default_value()) {
                /*check if a scope with the same ID exists*/
                if (scopeIndex.find(fullID) == scopeIndex.end()) {
                    if (fatherScope->strategy == strategy) {
                        pub = new InformationItem(fatherScope->strategy, fatherScope);
                        pub->updateIDs(pubIndex, fullID.substring(fullID.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN));
                        if (pub->updateSubscribers(fullID, _subscriber)) {
                            /*add the InformationItem to the subscriber's set*/
                            _subscriber->subscribedInformationItems.find_insert(fullID);
                            click_chatter("Intra-Node Rendezvous: added subscriber %s to (new) information item %s(%d)", _subscriber->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                            ret = SUCCESS;
                        } else {
                            ret = EXISTS;
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: Error could not add subscription - strategy mismatch");
                        ret = STRATEGY_MISMATCH;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: Error - cannot subscribe to info - a scope with the same ID exists");
                    ret = SCOPE_WITH_SAME_ID;
                }
            } else {
                if (fatherScope->strategy == strategy) {
                    if (pub->updateSubscribers(fullID, _subscriber)) {
                        /*add the scope to the publisher's set*/
                        _subscriber->subscribedInformationItems.find_insert(fullID);
                        /*do the rendez-vous process*/
                        RemoteHostSet subscribers;
                        pub->getSubscribers(subscribers);
                        /*careful here...I have multiple fathers*/
                        ScopeSetIter fathersc_it;
                        for (fathersc_it = pub->fatherScopes.begin(); fathersc_it != pub->fatherScopes.end(); fathersc_it++) {
                            (*fathersc_it).pointer->getSubscribers(subscribers);
                        }
                        pub->getPublishers(_publishers);
                        rendezvous(pub, _publishers, subscribers);
                        click_chatter("Intra-Node Rendezvous: added subscriber %s to information item %s(%d)", _subscriber->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                        ret = SUCCESS;
                    } else {
                        ret = EXISTS;
                    }
                } else {
                    click_chatter("Intra-Node Rendezvous: Error could not subscribe to InformationItem - strategy mismatch");
                    ret = STRATEGY_MISMATCH;
                }
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Error - Scope prefix %s doesn't exist", prefixID.quoted_hex().c_str());
            ret = FATHER_DOES_NOT_EXIST;
        }
    } else {
        ret = WRONG_IDS;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::unsubscribe_scope(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    Scope *sc;
    Scope *fatherScope;
    RemoteHostSet _publishers;
    RemoteHost *_subscriber = getRemoteHost(subscriberID);
    String fullID = prefixID + ID;
    sc = scopeIndex.get(fullID);
    if (sc != scopeIndex.default_value()) {
        if (sc->strategy == strategy) {
            fatherScope = scopeIndex.get(prefixID);
            if (fatherScope != scopeIndex.default_value()) {
                /*not a root scope*/
                RemoteHostPair * pair = sc->ids.get(fullID);
                /*erase _subscriber (if it exists) (second in the pair) from the appropriate ID pair*/
                if (pair->second.find(_subscriber) != pair->second.end()) {
                    pair->second.erase(_subscriber);
                    _subscriber->subscribedScopes.erase(fullID);
                    /*find all pieces of info that are affected by this and do the rendez-vous*/
                    InformationItemSet _informationitems;
                    sc->getInformationItems(_informationitems);
                    /*then, for each one do the rendez-vous process*/
                    for (InformationItemSetIter pub_it = _informationitems.begin(); pub_it != _informationitems.end(); pub_it++) {
                        RemoteHostSet subscribers;
                        (*pub_it).pointer->getSubscribers(subscribers);
                        /*careful here...I have multiple fathers*/
                        for (ScopeSetIter fathersc_it = (*pub_it).pointer->fatherScopes.begin(); fathersc_it != (*pub_it).pointer->fatherScopes.end(); fathersc_it++) {
                            (*fathersc_it).pointer->getSubscribers(subscribers);
                        }
                        (*pub_it).pointer->getPublishers(_publishers);
                        rendezvous((*pub_it).pointer, _publishers, subscribers);
                    }
                    /*do not try to delete if there are subscopes or InformationItems under the scope*/
                    if ((sc->childrenScopes.size() == 0) && (sc->informationitems.size() == 0)) {
                        /*different approach is followed depending on the number of father scopes (NOT on the number of IDS)*/
                        if (sc->fatherScopes.size() == 1) {
                            if (!sc->checkForOtherPubSub(fatherScope)) {
                                /*notify subscribers about deletion*/
                                RemoteHostSet subscribers;
                                StringSet _ids;
                                fatherScope->getSubscribers(subscribers);
                                sc->getIDs(_ids);
                                notifySubscribers(SCOPE_UNPUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                                /*safe to delete Scope*/
                                fatherScope->childrenScopes.erase(sc);
                                click_chatter("Intra-Node Rendezvous: deleted subscriber %s from (deleted) scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                                /*delete all IDs from scopeIndex*/
                                for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                                    scopeIndex.erase((*it).first);
                                }
                                delete sc;
                            } else {
                                click_chatter("Intra-Node Rendezvous: deleted subscriber %s from scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                            }
                        } else {
                            if (!sc->checkForOtherPubSub(fatherScope)) {
                                /*notify subscribers about deletion*/
                                RemoteHostSet subscribers;
                                StringSetIter _ids_it;
                                StringSet _ids;
                                fatherScope->getSubscribers(subscribers);
                                sc->getIDs(_ids);
                                /*I have to delete identifiers of all other branches!!!!!*/
                                for (_ids_it = _ids.begin(); _ids_it != _ids.end(); _ids_it++) {
                                    if ((*_ids_it).object.compare(fullID) != 0) {
                                        _ids.erase((*_ids_it).object);
                                    }
                                }
                                notifySubscribers(SCOPE_UNPUBLISHED, _ids, sc->strategy, sc->str_opt, sc->str_opt_len, subscribers);
                                /*safe to delete scope (only this the specific branch)*/
                                fatherScope->childrenScopes.erase(sc);
                                sc->fatherScopes.erase(fatherScope);
                                click_chatter("Intra-Node Rendezvous: deleted subscriber %s from (deleted) scope branch %s(%d)", _subscriber->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                                /*delete all IDs from scopeIndex*/
                                String suffixID = (*sc->ids.begin()).first.substring((*sc->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                                for (IdsHashMapIter it = fatherScope->ids.begin(); it != fatherScope->ids.end(); it++) {
                                    delete sc->ids.get((*it).first + suffixID);
                                    sc->ids.erase((*it).first + suffixID);
                                    scopeIndex.erase((*it).first + suffixID);
                                }
                            } else {
                                click_chatter("Intra-Node Rendezvous: deleted subscriber %s from scope branch %s(%d)", _subscriber->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                            }
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from scope %s(%d)", _subscriber->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                    }
                    /*SUCCESS means that the subscriber was removed from the scope*/
                    return SUCCESS;
                } else {
                    return DOES_NOT_EXIST;
                }
            } else {
                /*a ROOT scope*/
                RemoteHostPair * pair = sc->ids.get(fullID);
                /*erase _subscriber (if it exists) (second in the pair) from the appropriate ID pair*/
                if (pair->second.find(_subscriber) != pair->second.end()) {
                    pair->second.erase(_subscriber);
                    _subscriber->subscribedScopes.erase(fullID);
                    /*find all pieces of info that are affected by this and do the rendez-vous process*/
                    InformationItemSet _informationitems;
                    sc->getInformationItems(_informationitems);
                    /*then, for each one do the rendez-vous process*/
                    for (InformationItemSetIter pub_it = _informationitems.begin(); pub_it != _informationitems.end(); pub_it++) {
                        RemoteHostSet subscribers;
                        (*pub_it).pointer->getSubscribers(subscribers);
                        /*careful here...I have multiple fathers*/
                        ScopeSetIter fathersc_it;
                        for (fathersc_it = (*pub_it).pointer->fatherScopes.begin(); fathersc_it != (*pub_it).pointer->fatherScopes.end(); fathersc_it++) {
                            (*fathersc_it).pointer->getSubscribers(subscribers);
                        }
                        (*pub_it).pointer->getPublishers(_publishers);
                        rendezvous((*pub_it).pointer, _publishers, subscribers);
                    }
                    /*do not try to delete after unsubscribing if there are subscopes or informationitems under the scope*/
                    if ((sc->childrenScopes.size() == 0) && (sc->informationitems.size() == 0)) {
                        if (!sc->checkForOtherPubSub(NULL)) {
                            click_chatter("Intra-Node Rendezvous: deleted subscriber %s from (deleted) scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                            /*delete all IDs from scopeIndex*/
                            for (IdsHashMapIter it = sc->ids.begin(); it != sc->ids.end(); it++) {
                                scopeIndex.erase((*it).first);
                            }
                            delete sc;
                        } else {
                            click_chatter("Intra-Node Rendezvous: deleted subscriber %s from scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from scope %s(%d)", _subscriber->remoteHostID.c_str(), sc->printID().c_str(), (int) strategy);
                    }
                    /*SUCCESS means that the subscriber was removed from the scope*/
                    return SUCCESS;
                } else {
                    return DOES_NOT_EXIST;
                }
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Cannot Unsubscribe from scope %s..strategy mismatch", fullID.quoted_hex().c_str());
            ret = STRATEGY_MISMATCH;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: Scope %s does not exist...Unsubscribe from what?", fullID.quoted_hex().c_str());
        ret = SCOPE_DOES_NOT_EXIST;
    }
    return ret;
}

unsigned int IntraNodeRendezvous::unsubscribe_info(String &subscriberID, String &ID, String &prefixID, unsigned char &strategy, const void */*str_opt*/, unsigned int /*str_opt_len*/) {
    unsigned int ret;
    InformationItem *pub;
    Scope *fatherScope;
    RemoteHostSet _publishers;
    RemoteHost *_subscriber = getRemoteHost(subscriberID);
    String fullID = prefixID + ID;
    pub = pubIndex.get(fullID);
    fatherScope = scopeIndex.get(prefixID);
    if (pub != pubIndex.default_value()) {
        if (fatherScope->strategy == strategy) {
            RemoteHostPair * pair = pub->ids.get(fullID);
            /*erase _subscriber (if it exists) (second in the pair) from the appropriate ID pair*/
            if (pair->second.find(_subscriber) != pair->second.end()) {
                pair->second.erase(_subscriber);
                _subscriber->subscribedInformationItems.erase(fullID);
                /*do the rendez-vous if there are any left publishers and subscribers*/
                RemoteHostSet subscribers;
                pub->getSubscribers(subscribers);
                /*careful here...I have multiple fathers*/
                ScopeSetIter fathersc_it;
                for (fathersc_it = pub->fatherScopes.begin(); fathersc_it != pub->fatherScopes.end(); fathersc_it++) {
                    (*fathersc_it).pointer->getSubscribers(subscribers);
                }
                pub->getPublishers(_publishers);
                rendezvous(pub, _publishers, subscribers);
                /*different approach is followed depending on the number of father scopes (NOT on the number of IDS)*/
                if (pub->fatherScopes.size() == 1) {
                    if (!pub->checkForOtherPubSub(fatherScope)) {
                        /*safe to delete InformationItem*/
                        fatherScope->informationitems.erase(pub);
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from (deleted) information item %s(%d)", _subscriber->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                        /*delete all IDs from pubIndex*/
                        for (IdsHashMapIter it = pub->ids.begin(); it != pub->ids.end(); it++) {
                            pubIndex.erase((*it).first);
                        }
                        delete pub;
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from information item %s(%d)", _subscriber->remoteHostID.c_str(), pub->printID().c_str(), (int) strategy);
                    }
                } else {
                    if (!pub->checkForOtherPubSub(fatherScope)) {
                        /*safe to delete InformationItem*/
                        fatherScope->informationitems.erase(pub);
                        pub->fatherScopes.erase(fatherScope);
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from (deleted) information item branch %s(%d)", _subscriber->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                        /*delete all IDs from pubIndex*/
                        String suffixID = (*pub->ids.begin()).first.substring((*pub->ids.begin()).first.length() - PURSUIT_ID_LEN, PURSUIT_ID_LEN);
                        for (IdsHashMapIter it = fatherScope->ids.begin(); it != fatherScope->ids.end(); it++) {
                            /*since the pub is not deleted, manually delete this pair*/
                            delete pub->ids.get((*it).first + suffixID);
                            pub->ids.erase((*it).first + suffixID);
                            pubIndex.erase((*it).first + suffixID);
                        }
                    } else {
                        click_chatter("Intra-Node Rendezvous: deleted subscriber %s from information item branch %s(%d)", _subscriber->remoteHostID.c_str(), fullID.quoted_hex().c_str(), (int) strategy);
                    }
                }
                /*SUCCESS means that the subscriber was removed from the info*/
                return SUCCESS;
            } else {
                return DOES_NOT_EXIST;
            }
        } else {
            click_chatter("Intra-Node Rendezvous: Cannot Unsubscribe from %s..strategy mismatch", pub->printID().c_str());
            ret = STRATEGY_MISMATCH;
        }
    } else {
        click_chatter("Intra-Node Rendezvous: InformationItem %s does not exist...Unsubscribes from what", fullID.quoted_hex().c_str());
        ret = INFO_DOES_NOT_EXIST;
    }
    return ret;
}

void IntraNodeRendezvous::rendezvous(InformationItem *pub, RemoteHostSet &_publishers, RemoteHostSet &_subscribers) {
    /*I have a publication..it can have zero, one or many publishers..(check all ids)*/
    if (_publishers.size() > 0) {
        /*all publishers and subscribers should be running locally*/
        if (_subscribers.size() > 0) {
            /*we use the internal identifier just to have something instead of NULL - it's not actually used anywhere*/
            notifyPublishers(pub, START_PUBLISH);
        } else {
            notifyPublishers(pub, STOP_PUBLISH);
        }
    }
}

void IntraNodeRendezvous::notifyPublishers(InformationItem *pub, unsigned char type) {
    WritablePacket *p;
    unsigned int payload_len;
    int totalIDsLength = 0;
    unsigned char numberOfIDs;
    //click_chatter("Intra-Node Rendezvous: notify publishers about IDs:");
    numberOfIDs = (unsigned char) pub->ids.size();
    for (IdsHashMapIter it = pub->ids.begin(); it != pub->ids.end(); it++) {
        totalIDsLength = totalIDsLength + (*it).first.length();
        //click_chatter("Intra-Node Rendezvous: %s", (*it).first.quoted_hex().c_str());
    }
    payload_len = sizeof (numberOfIDs)+((int) numberOfIDs) * sizeof (unsigned char) +totalIDsLength + sizeof (pub->strategy) + sizeof (pub->str_opt_len) + pub->str_opt_len + sizeof (type);
    p = InClickAPI::prepare_publish_data(RV_LOCAL_IDENTIFIER, notificationIID, IMPLICIT_RENDEZVOUS, NULL, 0, payload_len);
    InClickAPI::add_data(p, &numberOfIDs, sizeof (numberOfIDs));
    IdsHashMapIter it = pub->ids.begin();
    for (int i = 0; i < (int) numberOfIDs; i++) {
        unsigned char IDLength = (unsigned char) it.key().length() / PURSUIT_ID_LEN;
        InClickAPI::add_data(p, &IDLength, sizeof (IDLength));
        InClickAPI::add_data(p, (*it).first.c_str(), (*it).first.length());
        it++;
    }
    InClickAPI::add_data(p, &pub->strategy, sizeof (pub->strategy));
    InClickAPI::add_data(p, &pub->str_opt_len, sizeof (pub->str_opt_len));
    InClickAPI::add_data(p, pub->str_opt, pub->str_opt_len);
    InClickAPI::add_data(p, &type, sizeof (type));
    rv_element->output(0).push(p);
}

void IntraNodeRendezvous::notifySubscribers(unsigned char notification_type, StringSet &IDs, unsigned char strategy, const void *str_opt, unsigned int str_opt_len, RemoteHostSet &subscribers) {
    WritablePacket *p;
    unsigned int payload_len;
    int totalIDsLength = 0;
    unsigned char numberOfIDs;
    /*only this node can be a subscriber - intra-node strategy*/
    if (subscribers.size() > 0) {
        //click_chatter("Intra-Node Rendezvous: notify subscribers about IDs:");
        numberOfIDs = (unsigned char) IDs.size();
        for (StringSetIter it = IDs.begin(); it != IDs.end(); it++) {
            totalIDsLength = totalIDsLength + (*it).object.length();
            //click_chatter("Intra-Node Rendezvous: %s", (*it).object.quoted_hex().c_str());
        }
        payload_len = sizeof (numberOfIDs)+((int) numberOfIDs) * sizeof (unsigned char) +totalIDsLength + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len + sizeof (notification_type);
        p = InClickAPI::prepare_publish_data(RV_LOCAL_IDENTIFIER, notificationIID, IMPLICIT_RENDEZVOUS, NULL, 0, payload_len);
        InClickAPI::add_data(p, &numberOfIDs, sizeof (numberOfIDs));
        StringSetIter it = IDs.begin();
        for (int i = 0; i < (int) numberOfIDs; i++) {
            unsigned char IDLength = (unsigned char) (*it).object.length() / PURSUIT_ID_LEN;
            InClickAPI::add_data(p, &IDLength, sizeof (IDLength));
            InClickAPI::add_data(p, (*it).object.c_str(), (*it).object.length());
            it++;
        }
        InClickAPI::add_data(p, &strategy, sizeof (strategy));
        InClickAPI::add_data(p, &str_opt_len, sizeof (str_opt_len));
        InClickAPI::add_data(p, str_opt, str_opt_len);
        InClickAPI::add_data(p, &notification_type, sizeof (notification_type));
        rv_element->output(0).push(p);
    }
}

RemoteHost * IntraNodeRendezvous::getRemoteHost(String & nodeID) {
    RemoteHost *_remotehost = NULL;
    _remotehost = pub_sub_Index.get(nodeID);
    if (_remotehost == pub_sub_Index.default_value()) {
        /*create a new _remotehost*/
        _remotehost = new RemoteHost(nodeID);
        pub_sub_Index.set(nodeID, _remotehost);
    }
    return _remotehost;
}

String IntraNodeRendezvous::listInfoStructs() {
    StringAccum sa (2048);

    ScopeSet scopes;
    InformationItemSet iitems;

    //Timestamp ts1 = Timestamp::now();

    /*
     * A JSON-like format is practical for "dumping" the information
     * structures as it is a commonly used standard format, has low
     * overhead and complexity, is easy to encode and parse, is
     * text-based and human-readable, and suits our purposes well.
     */

    sa << "{\"scopes\": [";
    for (ScopeHashMapIter hm_it = scopeIndex.begin(); hm_it; ++hm_it) {
        scopes.find_insert(PointerSetItem<Scope>(hm_it->second));
    }
    size_t s_i = 0, s_n = scopes.size() - 1;
    for (ScopeSetIter s_it = scopes.begin(); s_it; ++s_it) {
        IdsHashMap *ids = &s_it->pointer->ids;
        sa << "[";
        size_t id_i = 0, id_n = ids->size() - 1;
        for (IdsHashMapIter id_it = ids->begin(); id_it; ++id_it) {
            const String *id = &id_it->first;
            RemoteHostSet *ps = &id_it->second->first;
            RemoteHostSet *ss = &id_it->second->second;
            String hex_id = id->quoted_hex();
            if (hex_id.length() >= 3)
                hex_id = hex_id.substring(2, hex_id.length()-3);
            sa << "{\"id\": \"" << hex_id << "\", \"pub\": [";
            size_t rhs_i = 0, rhs_n = ps->size() - 1;
            for (RemoteHostSetIter rhs_it = ps->begin(); rhs_it; ++rhs_it) {
                String *rh_id = &rhs_it->pointer->remoteHostID;
                sa << '\"' << *rh_id << ((rhs_i++ == rhs_n) ? "\"": "\", ");
            }
            sa << "], \"sub\": [";
            rhs_i = 0, rhs_n = ss->size() - 1;
            for (RemoteHostSetIter rhs_it = ss->begin(); rhs_it; ++rhs_it) {
                String *rh_id = &rhs_it->pointer->remoteHostID;
                sa << '\"' << *rh_id << ((rhs_i++ == rhs_n) ? "\"": "\", ");
            }
            sa << ((id_i++ == id_n) ? "] } ": "] }, ");
        }
        sa << ((s_i++ == s_n) ? "] " : "], ");
    }

    sa << "], \"iitems\": [";
    for (IIHashMapIter hm_it = pubIndex.begin(); hm_it; ++hm_it) {
        iitems.find_insert(PointerSetItem<InformationItem>(hm_it->second));
    }
    s_i = 0, s_n = iitems.size() - 1;
    for (InformationItemSetIter s_it = iitems.begin(); s_it; ++s_it) {
        IdsHashMap *ids = &s_it->pointer->ids;
        sa << "[";
        size_t id_i = 0, id_n = ids->size() - 1;
        for (IdsHashMapIter id_it = ids->begin(); id_it; ++id_it) {
            const String *id = &id_it->first;
            RemoteHostSet *ps = &id_it->second->first;
            RemoteHostSet *ss = &id_it->second->second;
            String hex_id = id->quoted_hex();
            if (hex_id.length() >= 3)
                hex_id = hex_id.substring(2, hex_id.length()-3);
            sa << "{\"id\": \"" << hex_id << "\", \"pub\": [";
            size_t rhs_i = 0, rhs_n = ps->size() - 1;
            for (RemoteHostSetIter rhs_it = ps->begin(); rhs_it; ++rhs_it) {
                String *rh_id = &rhs_it->pointer->remoteHostID;
                sa << '\"' << *rh_id << ((rhs_i++ == rhs_n) ? "\"": "\", ");
            }
            sa << "], \"sub\": [";
            rhs_i = 0, rhs_n = ss->size() - 1;
            for (RemoteHostSetIter rhs_it = ss->begin(); rhs_it; ++rhs_it) {
                String *rh_id = &rhs_it->pointer->remoteHostID;
                sa << '\"' << *rh_id << ((rhs_i++ == rhs_n) ? "\"": "\", ");
            }
            sa << ((id_i++ == id_n) ? "] } ": "] }, ");
        }
        sa << ((s_i++ == s_n) ? "] " : "], ");
    }
    
    sa << "] }\r\n";

    //click_chatter((Timestamp::now() - ts1).unparse_interval().c_str());

    return sa.take_string();
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(IntraNodeRendezvous)
