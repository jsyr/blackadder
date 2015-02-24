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
#include "touser.hh"

CLICK_DECLS

ToUser::ToUser() {
}

ToUser::~ToUser() {
    click_chatter("ToUser: destroyed!");
}

int ToUser::configure(Vector<String> &conf, ErrorHandler */*errh*/) {
    fromuser_element = (FromUser *) cp_element(conf[0], this);
    return 0;
}

int ToUser::initialize(ErrorHandler */*errh*/) {
    return 0;
}

void ToUser::cleanup(CleanupStage /*stage*/) {
}

void ToUser::push(int, Packet *p) {
    WritablePacket *final_packet;
    struct nlmsghdr *nlh;
    unsigned int dest_pid;
    memcpy(&dest_pid, p->data(), sizeof (dest_pid));
    p->pull(sizeof (dest_pid));
    /*LocalProxy pushed a packet to be sent to an application*/
    final_packet = p->push(sizeof (struct nlmsghdr));
    /*Now it is ready - I have to create a netlink header and send it*/
    nlh = (struct nlmsghdr *) final_packet->data();
    nlh->nlmsg_len = sizeof (final_packet->length());
    nlh->nlmsg_type = 0;
    nlh->nlmsg_flags = 1;
    nlh->nlmsg_seq = 0;
#if CLICK_LINUXMODULE
    nlh->nlmsg_pid = 0;
#else
    nlh->nlmsg_pid = 9999;
#endif
    send_packet(final_packet, dest_pid);
}

#if CLICK_LINUXMODULE

void ToUser::send_packet(WritablePacket *final_packet, unsigned int dest_pid) {
    netlink_unicast(fromuser_element->nl_sk, final_packet->skb(), dest_pid, MSG_DONTWAIT);
}

#else

void ToUser::send_packet(WritablePacket *final_packet, unsigned int dest_pid) {
#if HAVE_USE_NETLINK
    struct sockaddr_nl d_nladdr;
#elif HAVE_USE_UNIX
    struct sockaddr_un d_unaddr;
#endif

    struct msghdr msg;
    struct iovec iov[1];

#if HAVE_USE_NETLINK
    memset(&d_nladdr, 0, sizeof (d_nladdr));
    d_nladdr.nl_family = AF_NETLINK;
    d_nladdr.nl_pad = 0;
    d_nladdr.nl_pid = dest_pid;
#elif HAVE_USE_UNIX
    memset(&d_unaddr, 0, sizeof (d_unaddr));
# ifndef __linux__
    d_unaddr.sun_len = sizeof (d_unaddr);
# endif
    d_unaddr.sun_family = PF_LOCAL;
    ba_id2path(d_unaddr.sun_path, dest_pid);
#endif

    iov[0].iov_base = final_packet->data();
    iov[0].iov_len = final_packet->length();
    memset(&msg, 0, sizeof (msg));
#if HAVE_USE_NETLINK
    msg.msg_name = (void *) &d_nladdr;
    msg.msg_namelen = sizeof (d_nladdr);
#elif HAVE_USE_UNIX
    msg.msg_name = (void *) &d_unaddr;
    msg.msg_namelen = sizeof (d_unaddr);
#endif
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    sendmsg(fromuser_element->fd, &msg, MSG_DONTWAIT);
    /*remove buffer from queue and free it*/
    final_packet->kill();
}

#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(ToUser)
ELEMENT_REQUIRES(FromUser)
