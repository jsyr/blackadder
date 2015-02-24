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
#include "rv.hh"
/*these must be inlcuded here and NOT In the .hh file*/
#include "intra_node_rendezvous.hh"
#include "intra_domain_rendezvous.hh"
/*****************************************************/

CLICK_DECLS

RV::RV() {

}

RV::~RV() {
    click_chatter("RV: Destroyed!");
}

int RV::configure(Vector<String> &conf, ErrorHandler *errh) {
    String TMFID_str;
    /*/FFFFFFFFFFFFFFFE*/
    const char tm_scope_base[PURSUIT_ID_LEN] = {255, 255, 255, 255, 255, 255, 255, 254};
    if (cp_va_kparse(conf, this, errh,
            "NODEID", cpkM, cpString, &nodeID,
            "TMFID", cpkN, cpString, &TMFID_str,
            cpEnd) < 0) {
        return -1;
    }
    TMIID = String(tm_scope_base, PURSUIT_ID_LEN) + nodeID;
    if (TMFID_str.length() != 0) {
        if (TMFID_str.length() != FID_LEN * 8) {
            errh->fatal("TMFID LID should be %d bits...it is %d bits", FID_LEN * 8, TMFID_str.length());
            return -1;
        }
        TMFID = BABitvector(FID_LEN * 8);
        for (int j = 0; j < TMFID_str.length(); j++) {
            if (TMFID_str.at(j) == '1') {
                TMFID[TMFID_str.length() - j - 1] = true;
            } else {
                TMFID[TMFID_str.length() - j - 1] = false;
            }
        }
    }
    click_chatter("*******************************************************RV ELEMENT CONFIGURATION*******************************************************");
    click_chatter("Node Label: %s", nodeID.c_str());
    click_chatter("TM Identifier: %s", TMIID.quoted_hex().c_str());
    click_chatter("Default LIPSIN Identifier to TM: %s", TMFID.to_string().c_str());
    return 0;
}

int RV::initialize(ErrorHandler */*errh*/) {
    intra_node_rv = new IntraNodeRendezvous(this);
    intra_domain_rv = new IntraDomainRendezvous(this);
    return 0;
}

void RV::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_ROUTER_INITIALIZED) {
        delete intra_node_rv;
        delete intra_domain_rv;
    }
}

void RV::push(int in_port, Packet * p) {
    unsigned int str_opt_len;
    unsigned char type, typeOfAPIEvent;
    unsigned char IDLengthOfAPIEvent;
    String IDOfAPIEvent;
    String ID, prefixID;
    String nodeID;
    const void *str_opt = NULL;
    unsigned char IDLength/*in fragments of PURSUIT_ID_LEN each*/, prefixIDLength/*in fragments of PURSUIT_ID_LEN each*/, strategy;
    if (in_port == 0) {
        typeOfAPIEvent = *(p->data());
        IDLengthOfAPIEvent = *(p->data() + sizeof (typeOfAPIEvent));
        IDOfAPIEvent = String((const char *) (p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent)), IDLengthOfAPIEvent * PURSUIT_ID_LEN);
        if (typeOfAPIEvent == PUBLISHED_DATA) {
            nodeID = IDOfAPIEvent.substring(PURSUIT_ID_LEN);
            type = *(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN);
            IDLength = *(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type));
            ID = String((const char *) (p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength)), IDLength * PURSUIT_ID_LEN);
            prefixIDLength = *(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length());
            prefixID = String((const char *) (p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength)), prefixIDLength * PURSUIT_ID_LEN);
            strategy = *(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length());
            str_opt_len = *(p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length() + sizeof (strategy));
            if (str_opt_len > 0) {
                /*str_opt is not allocated yet..*/
                str_opt = p->data() + sizeof (typeOfAPIEvent) + sizeof (IDLengthOfAPIEvent) + IDLengthOfAPIEvent * PURSUIT_ID_LEN + sizeof (type) + sizeof (IDLength) + ID.length() + sizeof (prefixIDLength) + prefixID.length() + sizeof (strategy) + sizeof (str_opt_len);
            }
            /*NOTE: str_opt and str_opt_len must be assigned on a per-publisher and subscriber basis..So a scope must have a single dissemination strategy
             *but each publisher and subscriber must be accompanied by the strategy options provided in the respective pub/sub request.
             *This way the rendezvous function can utilise this information in order to perform rendezvous.
             *TODO: THIS IS NOT IMPLEMENTED YET - str_opt and str_opt_len are passed to all respective functions but are left unused*/
            switch (strategy) {
                case NODE_LOCAL:
                    intra_node_rv->handleRVRequest(type, nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
                    break;
                case LINK_LOCAL:
                    click_chatter("RV: there is no rendezvous for LINK_LOCAL strategy --- killing packet and moving on");
                    break;
                case BROADCAST_IF:
                    click_chatter("RV: there is no rendezvous for BROADCAST_IF strategy --- killing packet and moving on");
                    break;
                case DOMAIN_LOCAL:
                    intra_domain_rv->handleRVRequest(type, nodeID, ID, prefixID, strategy, str_opt, str_opt_len);
                    break;
                case IMPLICIT_RENDEZVOUS:
                    click_chatter("RV: there is no rendezvous for BROADCAST_IF strategy --- killing packet and moving on");
                    break;
                default:
                    click_chatter("RV: a weird strategy that I don't know of --- killing packet and moving on");
                    break;
            }
        } else {
            click_chatter("RV: FATAL - I am expecting only PUBLISHED_DATA pub/sub events");
        }
    } else {
        click_chatter("RV: I am not expecting packets from other click ports - FATAL");
    }
    p->kill();
}

String RV::listInfoStructs(int t) {
    switch (t) {
        case NODE_LOCAL:
            return static_cast<IntraNodeRendezvous *>(intra_node_rv)->listInfoStructs();
        case DOMAIN_LOCAL:
            return static_cast<IntraDomainRendezvous *>(intra_domain_rv)->listInfoStructs();
        default:
            click_chatter("RV: listInfoStructs() not implemented for strategy %d", t);
            return "\r\n";
    }
}

static String RV_read_dump(Element *e, void *thunk) {
    RV *c = (RV *)e;
    return c->listInfoStructs((int)(uintptr_t)thunk);
}

void RV::add_handlers() {
    add_read_handler("dump_intra_node",
                     RV_read_dump, (int)NODE_LOCAL);
    add_read_handler("dump_intra_domain",
                     RV_read_dump, (int)DOMAIN_LOCAL);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(RV)
