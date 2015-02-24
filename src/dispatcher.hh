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
#ifndef CLICK_DISPATCHER_HH
#define CLICK_DISPATCHER_HH

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/router.hh>

#include "helper.hh"
#include "in_click_api.hh"
#include "ba_bitvector.hh"
#if CLICK_USERLEVEL
#include <signal.h>
#endif


CLICK_DECLS

class LocalHandlerInterface;

/**@brief (Blackadder Core) The Dispatcher Element is the core element in a Blackadder Node.
 * 
 * All Click packets received by the Core component are annotated with an application identifier by the FromNetlink Element. 
 * Click packets received by other Click elements are annotated with the Click port with which the Dispatcher element is connected.
 * A further role is that of providing a proxy function to all publishers and subscribers. 
 * Rendezvous nodes (even the one running in the same node) do not know about individual application identifiers or click elements.
 * Instead, a statistically unique node label that identifies the network node from which a request was sent is stored by the Dispatcher.
 */
class Dispatcher : public Element {
public:
    Dispatcher();
    ~Dispatcher();
    const char *class_name() const {return "Dispatcher";}
    const char *port_count() const {return "-/-";}
    const char *processing() const {return PUSH;}
    int configure(Vector<String>&, ErrorHandler*);
    int configure_phase() const {return 300;}
    int initialize(ErrorHandler *errh);
    void cleanup(CleanupStage stage);
    void push(int port, Packet *p);
    void handleLocalPublication(Packet *p, unsigned int local_identifier, String &ID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len);
    void handleLocalPubSubRequest(Packet *p, unsigned int local_identifier, unsigned char type, String &ID, String &prefixID, unsigned char strategy, const void *str_opt, unsigned int str_opt_len);
    void handleRVNotification(Packet *p);
    void handleNetworkPublication(unsigned char strategy, Vector<String> &IDs, Packet *p /*the packet has some headroom and only the data which hasn't been copied yet*/);
    void pushPubSubEvent(unsigned int local_identifier, unsigned char type, String ID);
    void pushDataEvent(unsigned int local_identifier, String &ID, Packet *p);
    void publishToNetwork(const void *forwarding_information, unsigned int forwarding_information_length, Vector<String> &IDs, unsigned char strategy, Packet *p);
    void disconnect(unsigned int local_identifier);
    LocalHandlerInterface *intra_node_local_handler;
    LocalHandlerInterface *link_local_handler;
    LocalHandlerInterface *intra_domain_local_handler;
    LocalHandlerInterface *implicit_rendezvous_local_handler;
    String nodeID;
    String notificationIID;
    String nodeRVIID;
    BABitvector defaultRV_dl;
};

CLICK_ENDDECLS
#endif
