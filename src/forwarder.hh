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

#ifndef CLICK_FORWARDER_HH
#define CLICK_FORWARDER_HH

#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/element.hh>
#include <click/hashtable.hh>

#include "helper.hh"

CLICK_DECLS

class ForwardingInterface;

/**@brief (Blackadder Core) The Forwarder Element implements the forwarding function. Currently it supports the basic LIPSIN mechanism.
 * 
 * It can work in two modes. In a MAC mode it expects ethernet frames from the network devices. It checks the LIPSIN identifiers and pushes packets to another Ethernet interface or to the LocalProxy.
 * In IP mode, the Forwarder expects raw IP sockets as the underlying network. Note that a mixed mode is currently not supported. Some lines must be written.
 */
class Forwarder : public Element {
public:
    /**
     * @brief Constructor: it does nothing - as Click suggests
     * @return 
     */
    Forwarder();
    /**
     * @brief Destructor: it does nothing - as Click suggests
     * @return 
     */
    ~Forwarder();
    /**
     * @brief the class name - required by Click
     * @return 
     */
    const char *class_name() const {return "Forwarder";}
    /**
     * @brief the port count - required by Click - it can have multiple output ports that are connected with Click "network" Elements, like ToDevice and raw sockets. 
     * It can have multiple input ports from multiple "network" devices, like FromDevice or raw sockets.
     * @return 
     */
    const char *port_count() const {return "-/-";}
    /**
     * @brief a PUSH Element.
     * @return PUSH
     */
    const char *processing() const {return PUSH;}
    /**
     * @brief Element configuration.
     * number of links and then for each link:
     * |strategy|click output port|address type|source address|destination address|forwarding information|
     */
    int configure(Vector<String>&, ErrorHandler*);
    /**@brief
     * @return the correct number so that it is configured afterwards
     */
    int configure_phase() const{return 100;}
    /**
     * @brief This method is called by Click when the Element is about to be initialized.
     * @param errh
     * @return 
     */
    int initialize(ErrorHandler *errh);
    /**@brief Cleanups everything. 
     * 
     * If stage >= CLEANUP_CONFIGURED (i.e. the Element was configured), Forwarder will delete all stored Forwarding Entries for all dissemination strategies.
     */
    void cleanup(CleanupStage stage);
    /**@brief This method is called whenever a packet is received from the network (and pushed to the Forwarder by a "network" Element) or whenever the Dispatcher pushes a packet to the Forwarder.
     * 
     * Dispatcher pushes packets to the 0 port of the Forwarder.
     * @param port the port from which the packet was pushed. 0 for LocalProxy, >0 for network elements
     * @param p a pointer to the packet
     */
    void push(int port, Packet *p);
    /**@brief It is used for filling the ip_id field in the IP packet when sending over raw sockets. It is increased for every sent packet.
     */
    atomic_uint32_t _id;
    /**@brief The Ethernet protocol type (hardcoded to be 0x080a)
     */
    int proto_type;
    ForwardingInterface *link_local_forwarding;
    ForwardingInterface *lipsin_forwarding;
    HashTable<int, int> port_types;
};

CLICK_ENDDECLS
#endif
