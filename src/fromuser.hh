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
#ifndef CLICK_FROM_USER_HH
#define CLICK_FROM_USER_HH

#include <click/config.h>
#include <click/element.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/task.hh>

#ifdef __linux__
#define HAVE_USE_NETLINK 1
#endif
#if !HAVE_USE_NETLINK
#define HAVE_USE_UNIX 1
#endif

#if CLICK_LINUXMODULE
#define NETLINK_BADDER 30
#include <click/hashtable.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/cxxprotect.h>
CLICK_CXX_PROTECT
#include <linux/netlink.h>
#include <linux/module.h>
#include <net/sock.h>
CLICK_CXX_UNPROTECT
#include <click/cxxunprotect.h>
#else
#include <click/cxxprotect.h>
CLICK_CXX_PROTECT
# if HAVE_USE_NETLINK
#include <linux/netlink.h>
# elif HAVE_USE_UNIX
#  ifdef __linux__
#include <linux/un.h>
#  else
#include <sys/un.h>
#  endif
# endif
CLICK_CXX_UNPROTECT
#include <click/cxxunprotect.h>
#endif

CLICK_DECLS

/**
 * @brief (Blackadder Core) The FromUser Element receives packets from applications, annotates them .
 * 
 * In kernel space it also keeps track of the applications (assuming that the netlink port used by each application is the process ID). 
 * When an application terminates, it sends a disconnection message to the Dispatcher on behalf of the application.
 * In user-space the application must send the message by itself before it terminates (or crashes?How?).
 * 
 * FromUser uses a kernel mutex when running in kernel space since packets are received in the context of the process and then are processed by a Click Task. 
 * A queue that is secured with the mutex is used for that transition.
 */
class FromUser : public Element {
public:
    /**
     * @brief Constructor: it does nothing - as Click suggests
     * @return 
     */
    FromUser();
    /**
     * @brief Destructor: it does nothing - as Click suggests
     * @return 
     */
    ~FromUser();

    /**
     * @brief the class name - required by Click
     * @return 
     */
    const char *class_name() const {return "FromUser";}

    /**
     * @brief the port count - required by Click - there is no input and single output to a Classifier.
     * @return 
     */
    const char *port_count() const {return "0/1";}

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
     * 
     * @return the correct number so that it is configured afterwards
     */
    int configure_phase() const {return 100;}
    /**
     * @brief This method is called by Click when the Element is about to be initialized.
     * 
     * In user space this method calls the add_select method to denote that the selected method should be called whenever the socket is readable.
     * In kernel space, it initializes the kernel socket (netlink port 0 and netlink protocol NETLINK_BLACKADDER) and the mutex that protects the down_queue.
     * In kernel space it also initializes the Task. The Task is scheduled only when packets arrive from the socket.
     * @param errh
     * @return 
     */
    int initialize(ErrorHandler *errh);
    /**@brief Cleanups everything.
     * 
     * If the stage is before CLEANUP_INITIALIZED (i.e. the element was never initialized), then it does nothing.
     * In the opposite case it unschedules the Task, clears, empties the down_queue and deletes all packets in it.
     * @param stage stage passed by Click
     */
    void cleanup(CleanupStage stage);
#if CLICK_LINUXMODULE
    /** the struct socket *, which represents the kernel netlink socket
     */
    struct sock *nl_sk;
#else
    /**@brief The selected method overrides Click Element's selected method (User-Space only).
     *  The netlink socket is always marked as readable. Whenever it is, the selected method is called.
     *  It reads a packet from the socket buffer (if possible), annotates it using the source netlink port and pushes it to a protocol Classifier.
     */
    void selected(int fd, int mask);
    /**the netlink socket descriptor
     */
    int fd;
# if HAVE_USE_UNIX
    struct sockaddr_un s_unaddr;
# endif
#endif
};

CLICK_ENDDECLS

#ifndef __LINUX_NETLINK_H
extern "C" {
struct nlmsghdr {
    uint32_t    nlmsg_len;
    uint16_t    nlmsg_type;
    uint16_t    nlmsg_flags;
    uint32_t    nlmsg_seq;
    uint32_t    nlmsg_pid;
};

struct sockaddr_nl {
    sa_family_t     nl_family;
    unsigned short  nl_pad;
    uint32_t        nl_pid;
    uint32_t        nl_groups;
};
}
#endif

#if HAVE_USE_UNIX
#define ba_id2path(path, id)  \
    snprintf((path), 100, "/tmp/blackadder.%05u", (id))
#endif

#endif /* CLICK_NETLINK_HH */
