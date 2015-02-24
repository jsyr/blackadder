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
#ifndef CLICK_TO_USER_HH
#define CLICK_TO_USER_HH

#include "fromuser.hh"

CLICK_DECLS

/**@brief (Blackadder Core) The ToUser Element is the Element that sends packets to applications.
 * 
 * The Dispatcher pushes annotated packets to the ToUser element, which then sends them to the right applications using the provided packet annotation.
 */
class ToUser : public Element {
public:
    /**
     * @brief Constructor: it does nothing - as Click suggests
     * @return 
     */
    ToUser();
    /**
     * @brief Destructor: it does nothing - as Click suggests
     * @return 
     */
    ~ToUser();
    /**
     * @brief the class name - required by Click
     * @return 
     */
    const char *class_name() const {return "ToUser";}
    /**
     * @brief the port count - required by Click - there is no output and a single input where  LocalProxy pushes packets.
     * @return 
     */
    const char *port_count() const {return "1/0";}
    /**
     * @brief a PUSH Element.
     * @return PUSH
     */
    const char *processing() const {return PUSH;}
    /**
     * @brief Element configuration - the base netlink socket is passed as the only parameter (in the Click configuration file)
     */
    int configure(Vector<String>&, ErrorHandler*);
    /**@brief This Element must be configured AFTER the base ApplicationInterface Element
     * @return the correct number so that it is configured afterwards
     */
    int configure_phase() const {return 101;}
    /**
     * @brief This method is called by Click when the Element is about to be initialized.
     * 
     * In kernel space it allocates and initializes the task that is later scheduled when packets are pushed from the LocalProxy.
     * It also allocates the mutex that protects the up_queue.
     * @param errh
     * @return 
     */
    int initialize(ErrorHandler *errh);
    /**@brief Cleanups everything.
     * 
     * If the stage is before CLEANUP_INITIALIZED (i.e. the element was never initialized), then it does nothing.
     * In the opposite case it unschedules the Task, empties the up_queue and deletes all packets in it.
     * @param stage passed by Click
     */
    void cleanup(CleanupStage stage);
    /**@brief the push method is called by the element connected to the ToUser element (i.e. the LocalProxy) and pushes a packet.
     * 
     * This method pushes some space in the packet so that the netlink header can fit. It then adds the header.
     * In user space the nlh->nlmsg_pid is assigned to 9999 whereas in kernel space is assigned to 0.
     * In user space the packet is pushed in the out_buf_queue and socket is registered for writing using the add_select.
     * In kernel space it is pushed in the up_queue and the Task is rescheduled.
     * @param port the port from which the packet was pushed
     * @param p a pointer to the packet
     */
    void push(int port, Packet *p);
    void send_packet(WritablePacket *final_packet,unsigned int dest_pid);
    /** @brief a pointer to the Base ApplicationInterface Element.
     */
    FromUser *fromuser_element;
};

CLICK_ENDDECLS
#endif
