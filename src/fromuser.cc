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

#include "fromuser.hh"

#if HAVE_USE_UNIX
#include <click/cxxprotect.h>
CLICK_CXX_PROTECT
# ifndef __APPLE__
#include <sys/ioctl.h>
# endif
CLICK_CXX_UNPROTECT
#include <click/cxxunprotect.h>
#endif

CLICK_DECLS

#if CLICK_LINUXMODULE
static FromUser *_fromuser;
#else
char fake_buf[1];
#endif

#if CLICK_LINUXMODULE

void nl_callback(struct sk_buff *skb) {
    int type, flags, nlmsglen, skblen;
    struct nlmsghdr *nlh;
    skblen = skb->len;
    if (skblen < sizeof (*nlh)) {
        return;
    }
    nlh = nlmsg_hdr(skb);
    nlmsglen = nlh->nlmsg_len;
    if (nlmsglen < sizeof (*nlh) || skblen < nlmsglen) {
        return;
    }
    flags = nlh->nlmsg_flags;
    type = nlh->nlmsg_type;
#if HAVE_SKB_DST_DROP
    skb_dst_drop(skb);
#else
    if (skb->dst) {
        dst_release(skb->dst);
        skb->dst = 0;
    }
#endif
    skb_orphan(skb);
    skb = skb_realloc_headroom(skb, 50);
    Packet *p = Packet::make(skb);
    p->pull(sizeof (struct nlmsghdr));
    /*push the packet to the only output, a protocol Classifier*/
    _fromuser->output(0).push(p);
}
#endif

FromUser::FromUser() {
}

FromUser::~FromUser() {
    click_chatter("FromUser: destroyed!");
}

int FromUser::configure(Vector<String> &/*conf*/, ErrorHandler */*errh*/) {
    return 0;
}

int FromUser::initialize(ErrorHandler *) {
#if CLICK_LINUXMODULE
    _fromuser = this;
    nl_sk = netlink_kernel_create(&init_net, NETLINK_BADDER, 0, nl_callback, NULL, THIS_MODULE);
    if (nl_sk == NULL) {
        return -1;
    }
#else
    int ret;

# if HAVE_USE_NETLINK
    struct sockaddr_nl s_nladdr;
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
# elif HAVE_USE_UNIX
    fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
#  ifdef __APPLE__
    int bufsize = 229376; /* tweak */
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
#  endif
# endif

    /* source address */
# if HAVE_USE_NETLINK
    memset(&s_nladdr, 0, sizeof (s_nladdr));
    s_nladdr.nl_family = AF_NETLINK;
    s_nladdr.nl_pad = 0;
    s_nladdr.nl_pid = 9999;
    ret = bind(fd, (struct sockaddr*) &s_nladdr, sizeof (s_nladdr));
# elif HAVE_USE_UNIX
    memset(&s_unaddr, 0, sizeof (s_unaddr));
#  ifndef __linux__
    s_unaddr.sun_len = sizeof (s_unaddr);
#  endif
    s_unaddr.sun_family = PF_LOCAL;
    strncpy(s_unaddr.sun_path, "/tmp/blackadder.09999", 22);
    if (unlink(s_unaddr.sun_path) != 0 && errno != ENOENT)
        perror("unlink");
#  ifdef __linux__
    ret = bind(fd, (struct sockaddr *) &s_unaddr, sizeof (s_unaddr));
#  else
    ret = bind(fd, (struct sockaddr *) &s_unaddr, SUN_LEN(&s_unaddr));
#  endif
# endif
    if (ret == -1) {
        perror("bind: ");
        return -1;
    }
    add_select(fd, SELECT_READ);
#endif
    return 0;
}

void FromUser::cleanup(CleanupStage stage) {
    if (stage >= CLEANUP_INITIALIZED) {
#if CLICK_LINUXMODULE
        /*release the socket*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
        if (nl_sk != NULL)
            sock_release(nl_sk->sk_socket);
#else
        netlink_kernel_release(nl_sk);
#endif
#else
        close(fd);
# if HAVE_USE_UNIX
        unlink(s_unaddr.sun_path);
# endif
#endif
    }
}

#if CLICK_USERLEVEL

void FromUser::selected(int fd, int mask) {
    WritablePacket *newPacket;
    int total_buf_size = -1;
    int bytes_read;
    if ((mask & SELECT_READ) == SELECT_READ) {
        /*read from the socket*/
# ifdef __linux__
        total_buf_size = recv(fd, fake_buf, 1, MSG_PEEK | MSG_TRUNC | MSG_WAITALL);
# else
#  ifdef __APPLE__
        socklen_t _option_len = sizeof(total_buf_size);
        if (getsockopt(fd, SOL_SOCKET, SO_NREAD, &total_buf_size, &_option_len) < 0) {
            click_chatter("getsockopt: %d", errno);
            return;
        }
#  else
        /* XXX: The FIONREAD ioctl gets the size of the whole unread buffer. */
        if (ioctl(fd, FIONREAD, &total_buf_size) < 0) {
            click_chatter("ioctl: %d", errno);
            return;
        }
#  endif
# endif
        if (total_buf_size < 0) {
            click_chatter("Hmmm");
            return;
        }
        newPacket = Packet::make(100, NULL, total_buf_size, 100);
        bytes_read = recv(fd, newPacket->data(), newPacket->length(), MSG_WAITALL);
        if (bytes_read > 0) {
            if ((uint32_t) bytes_read < newPacket->length()) {
                /* truncate to actual length (if total_buf_size > bytes_read despite MSG_WAITALL) */
                newPacket->take(newPacket->length() - bytes_read);
            }
            newPacket->pull(sizeof (struct nlmsghdr));
            /*push the packet to the only output, a protocol Classifier*/
            output(0).push(newPacket);
        } else {
            /* recv() returns -1 on error. We treat 0 and -N similarly. */
            click_chatter("recv returned %d", bytes_read);
            newPacket->kill();
        }
    }
}
#endif

CLICK_ENDDECLS
EXPORT_ELEMENT(FromUser)
