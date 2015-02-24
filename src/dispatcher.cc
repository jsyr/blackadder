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
#include "dispatcher.hh"
/*these must be inlcuded here and NOT In the .hh file*/
#include "intra_node_local_handler.hh"
#include "link_local_handler.hh"
#include "intra_domain_local_handler.hh"
#include "implicit_rendezvous_local_handler.hh"

/*****************************************************/

CLICK_DECLS

Element *this_element;

#if CLICK_USERLEVEL

void sigfun(int) {
    this_element->router()->please_stop_driver();
}
#endif

Dispatcher::Dispatcher() {
}

Dispatcher::~Dispatcher() {
    click_chatter("Dispatcher: destroyed!");
}

int Dispatcher::configure(Vector<String> &conf, ErrorHandler *errh) {
    String defRVFID;
    /*/FFFFFFFFFFFFFFFF*/
    const char rv_scope_base[PURSUIT_ID_LEN] = {255, 255, 255, 255, 255, 255, 255, 255};
    /*/FFFFFFFFFFFFFFFD*/
    const char notification_scope_base[PURSUIT_ID_LEN] = {255, 255, 255, 255, 255, 255, 255, 253};
    if (cp_va_kparse(conf, this, errh,
            "NODEID", cpkM, cpString, &nodeID,
            "DEFAULTRV", cpkM, cpString, &defRVFID,
            cpEnd) < 0) {
        return -1;
    }
    notificationIID = String(notification_scope_base, PURSUIT_ID_LEN) + nodeID;
    nodeRVIID = String(rv_scope_base, PURSUIT_ID_LEN) + nodeID;
    if (defRVFID.length() != FID_LEN * 8) {
        errh->fatal("defaultRV_dl should be %d bits...it is %d bits", FID_LEN * 8, defRVFID.length());
        return -1;
    }
    defaultRV_dl = BABitvector(FID_LEN * 8);
    for (int j = 0; j < defRVFID.length(); j++) {
        if (defRVFID.at(j) == '1') {
            defaultRV_dl[defRVFID.length() - j - 1] = true;
        } else {
            defaultRV_dl[defRVFID.length() - j - 1] = false;
        }
    }
    click_chatter("*****************************************************DISPATCHER CONFIGURATION*****************************************************");
    click_chatter("Node Label: %s", nodeID.c_str());
    click_chatter("Rendezvous Identifier: %s", nodeRVIID.quoted_hex().c_str());
    click_chatter("Notification Identifier: %s", notificationIID.quoted_hex().c_str());
    click_chatter("Default LIPSIN Identifier to RV: %s", defaultRV_dl.to_string().c_str());
    return 0;
}

int Dispatcher::initialize(ErrorHandler */*errh*/) {
    this_element = this;
#if CLICK_USERLEVEL
    (void) signal(SIGINT, sigfun);
#endif
    intra_node_local_handler = new IntraNodeLocalHandler(this);
    link_local_handler = new LinkLocalHandler(this);
    intra_domain_local_handler = new IntraDomainLocalHandler(this);
    implicit_rendezvous_local_handler = new ImplicitRendezvousLocalHandler(this);
    return 0;
}

void Dispatcher::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_ROUTER_INITIALIZED) {
        delete intra_node_local_handler;
        delete link_local_handler;
        delete intra_domain_local_handler;
        delete implicit_rendezvous_local_handler;
    }
}

void Dispatcher::push(int in_port, Packet * p) {
    unsigned int local_identifier = 0;
    unsigned int index = 0;
    unsigned char type, numberOfIDs, IDLength /*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength /*in fragments of PURSUIT_ID_LEN each*/, strategy;
    Vector<String> IDs;
    String ID, prefixID;
    const void *str_opt = NULL;
    unsigned int str_opt_len = 0;
    if (in_port == 1) {
        /*from port 1 I receive publications from the network*/
        strategy = *(p->data());
        index = 0;
        /*read the "header"*/
        numberOfIDs = *(p->data() + sizeof (strategy));
        /*Read all the identifiers*/
        for (int i = 0; i < (int) numberOfIDs; i++) {
            IDLength = *(p->data() + sizeof (strategy) + sizeof (numberOfIDs) + index);
            IDs.push_back(String((const char *) (p->data() + sizeof (strategy) + sizeof (numberOfIDs) + sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN));
            index = index + sizeof (IDLength) + IDLength * PURSUIT_ID_LEN;
        }
        /*remove the header*/
        p->pull(sizeof (strategy) + sizeof (numberOfIDs) + index);
        if ((IDs.size() == 1) && (IDs[0].compare(notificationIID) == 0)) {
            /*a special case here: Got back an RV/TM event from A topology Manager in the Network...it was published using the ID /FFFFFFFFFFFFFFFD/MYNODEID*/
            handleRVNotification(p);
        } else {
            /*a regular network publication..I will look for local subscribers*/
            /*Careful: I will not kill the packet - I will reuse it one way or another, so....get rid of everything except the data*/
            handleNetworkPublication(strategy, IDs, p);
        }
    } else {
        /*The request comes from user space or a Click Element (e.g. RV or a "higher layer" protocol). */
        memcpy(&local_identifier, p->data(), sizeof (local_identifier));
        type = *(p->data() + sizeof (local_identifier));
        if (type == DISCONNECT) {
            disconnect(local_identifier);
            p->kill();
        } else if (type == PUBLISH_DATA) {
            /*this is a publication coming from an application or a click element*/
            IDLength = *(p->data() + sizeof (local_identifier) + sizeof (type));
            ID = String((const char *) (p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
            strategy = *(p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length());
            str_opt_len = *(p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy));
            if (str_opt_len > 0) {
                /*str_opt is not allocated. it just points to the right memory in the user request. Must be copied if needs to be stored*/
                str_opt = p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy) + sizeof (str_opt_len);
            }
            p->pull(sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len);
            if ((ID.compare(notificationIID) == 0)) {
                /*A special case here: The RV element or the TM, which runs locally, published data using the blackadder API (published using the ID /FFFFFFFFFFFFFFFD/MYNODEID). This data is an RV notification*/
                handleRVNotification(p);
            } else {
                handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            }
        } else {
            /*read user's pub/sub request: ID, prefixID, strategy, strategy options*/
            IDLength = *(p->data() + sizeof (local_identifier) + sizeof (type));
            ID = String((const char *) (p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
            prefixIDLength = *(p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length());
            prefixID = String((const char *) (p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength)), prefixIDLength * PURSUIT_ID_LEN);
            strategy = *(p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length());
            str_opt_len = *(p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length() + sizeof (strategy));
            /*str_opt is not allocated. it just points to the right memory in the user request (at the end if there are no options). Must be copied if needs to be stored*/
            if (str_opt_len > 0) {
                str_opt = p->data() + sizeof (local_identifier) + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length() + sizeof (strategy) + sizeof (str_opt_len);
            }
            p->pull(sizeof (local_identifier));
            handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
        }
    }
}

void Dispatcher::handleRVNotification(Packet *p /*this is the payload of the RV notification*/) {
    unsigned char numberOfIDs, IDLength/*in fragments of PURSUIT_ID_LEN each*/, strategy;
    unsigned int index = 0, str_opt_len = 0;
    const void *str_opt = NULL;
    Vector<String> IDs;
    numberOfIDs = *(p->data());
    for (int i = 0; i < (int) numberOfIDs; i++) {
        IDLength = *(p->data() + sizeof (numberOfIDs) + index);
        IDs.push_back(String((const char *) (p->data() + sizeof (numberOfIDs) + sizeof (IDLength) + index), IDLength * PURSUIT_ID_LEN));
        index = index + sizeof (IDLength) + IDLength*PURSUIT_ID_LEN;
    }
    strategy = *(p->data() + sizeof (numberOfIDs) + index);
    str_opt_len = *(p->data() + sizeof (numberOfIDs) + index + sizeof (strategy));
    if (str_opt_len > 0) {
        str_opt = p->data() + sizeof (numberOfIDs) + index + sizeof (strategy) + sizeof (str_opt_len);
    }
    p->pull(sizeof (numberOfIDs) + index + sizeof (strategy) + sizeof (str_opt_len) + str_opt_len);
    switch (strategy) {
        case NODE_LOCAL:
            intra_node_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case LINK_LOCAL:
            link_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case BROADCAST_IF:
            link_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case DOMAIN_LOCAL:
            intra_domain_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case IMPLICIT_RENDEZVOUS:
            implicit_rendezvous_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
            implicit_rendezvous_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_LOCAL:
            implicit_rendezvous_local_handler->handleRVNotification(IDs, strategy, str_opt_len, str_opt, p /*p now has only the type and any extra data with the notification*/);
            break;
        default:
            click_chatter("Dispatcher: Unknown strategy %d in the RV notification - killing the packet", strategy);
            break;
    }
    p->kill();
}

void Dispatcher::handleNetworkPublication(unsigned char strategy, Vector<String> &IDs, Packet *p /*only data*/) {
    switch (strategy) {
        case NODE_LOCAL:
            intra_node_local_handler->handleNetworkPublication(IDs, p);
            break;
        case LINK_LOCAL:
            link_local_handler->handleNetworkPublication(IDs, p);
            break;
        case BROADCAST_IF:
            link_local_handler->handleNetworkPublication(IDs, p);
            break;
        case DOMAIN_LOCAL:
            intra_domain_local_handler->handleNetworkPublication(IDs, p);
            break;
        case IMPLICIT_RENDEZVOUS:
            implicit_rendezvous_local_handler->handleNetworkPublication(IDs, p);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
            implicit_rendezvous_local_handler->handleNetworkPublication(IDs, p);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_LOCAL:
            implicit_rendezvous_local_handler->handleNetworkPublication(IDs, p);
            break;
        default:
            click_chatter("Dispatcher: handleNetworkPublication: unknown strategy %d - killing packet..", strategy);
            p->kill();
            break;
    }
}

void Dispatcher::handleLocalPublication(Packet *p, unsigned int local_identifier, String &ID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    switch (strategy) {
        case NODE_LOCAL:
            intra_node_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case LINK_LOCAL:
            link_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case BROADCAST_IF:
            link_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case DOMAIN_LOCAL:
            intra_domain_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS:
            implicit_rendezvous_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
            implicit_rendezvous_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_LOCAL:
            implicit_rendezvous_local_handler->handleLocalPublication(p, local_identifier, ID, strategy, str_opt, str_opt_len);
            break;
        default:
            click_chatter("Dispatcher: handleLocalPublication: unknown strategy %d - killing packet..", strategy);
            p->kill();
            break;
    }
}

void Dispatcher::handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len) {
    switch (strategy) {
        case NODE_LOCAL:
            intra_node_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case LINK_LOCAL:
            link_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case BROADCAST_IF:
            link_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case DOMAIN_LOCAL:
            intra_domain_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS:
            implicit_rendezvous_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_DOMAIN:
            implicit_rendezvous_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        case IMPLICIT_RENDEZVOUS_ALGID_LOCAL:
            implicit_rendezvous_local_handler->handleLocalPubSubRequest(p, local_identifier, type, ID, prefixID, strategy, str_opt, str_opt_len);
            break;
        default:
            click_chatter("Dispatcher: handleLocalPubSubRequest: unknown strategy %d - killing packet..", strategy);
            p->kill();
            break;
    }
}

void Dispatcher::pushPubSubEvent(unsigned int local_identifier, unsigned char type, String ID) {
    WritablePacket *p;
    p = InClickAPI::prepare_event(local_identifier, type, ID, (unsigned int) 0);
    output(0).push(p);
}

void Dispatcher::pushDataEvent(unsigned int local_identifier, String &ID, Packet *p /*p contains only the data and has some headroom as well*/) {
    WritablePacket *newPacket;
    newPacket = InClickAPI::prepare_event(local_identifier, PUBLISHED_DATA, ID, p);
    output(0).push(newPacket);
}

void Dispatcher::publishToNetwork(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, Packet *network_publication) {
    WritablePacket *publication_packet;
    publication_packet = InClickAPI::prepare_network_publication(forwarding_information, forwarding_information_length, IDs, strategy, network_publication);
    output(1).push(publication_packet);
}

void Dispatcher::disconnect(unsigned int local_identifier) {
    /*for all different dissemination strategies that I keep state in the local handlers, I have to call the disconnect methods 
     * (probably I need to keep a vector with all supported handlers using polymorphism)*/
    click_chatter("Dispatcher: Entity %d disconnected...cleaning...", local_identifier);
    intra_node_local_handler->handleLocalDisconnection(local_identifier);
    link_local_handler->handleLocalDisconnection(local_identifier);
    intra_domain_local_handler->handleLocalDisconnection(local_identifier);
    implicit_rendezvous_local_handler->handleLocalDisconnection(local_identifier);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Dispatcher)
