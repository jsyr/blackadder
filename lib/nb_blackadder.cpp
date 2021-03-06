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

#include "nb_blackadder.h"

nb_blackadder* nb_blackadder::m_pInstance = NULL;

fd_set nb_blackadder::read_set;
fd_set nb_blackadder::write_set;
int nb_blackadder::pipe_fds[2] =
{ -1, -1 };
int nb_blackadder::sock_fd = -1;

queue<struct msghdr> nb_blackadder::output_queue;
pthread_t nb_blackadder::selector_thread;
pthread_mutex_t nb_blackadder::selector_mutex;
pthread_cond_t nb_blackadder::queue_overflow_cond;

queue<event *> nb_blackadder::event_queue;
pthread_t nb_blackadder::worker_thread;
pthread_mutex_t nb_blackadder::worker_mutex;
pthread_cond_t nb_blackadder::worker_cond;

char nb_blackadder::pipe_buf[1];
char nb_blackadder::fake_buf[1];

bool workerShouldEnd = false;
bool selectorShouldEnd = false;

#if HAVE_USE_NETLINK
struct sockaddr_nl nb_blackadder::s_nladdr, nb_blackadder::d_nladdr;
#elif HAVE_USE_UNIX
struct sockaddr_un nb_blackadder::s_nladdr, nb_blackadder::d_nladdr;
#endif

callbacktype nb_blackadder::cf = NULL;

/**@relates NB_Blackadder
 * @brief This is the default Callback that will be whenever an event is received if the application hasn't registered its own callback.
 * 
 * @param ev A pointer to an already constructed Event
 */
void
default_callback (event *ev)
{
  delete ev;
  cout << "ATTENTION - user did not specify a callback function for handling events...I am falling back to default and just deleting the Event" << endl;
}

void
nb_blackadder::signal_handler (int /*sig*/)
{
  int ret;
  cerr << "interrupted..." << endl;
  pthread_mutex_lock (&selector_mutex);
  selectorShouldEnd = true;
  pthread_mutex_unlock (&selector_mutex);
  ret = write (pipe_fds[1], pipe_buf, 1);
  pthread_mutex_lock (&worker_mutex);
  workerShouldEnd = true;
  pthread_cond_signal (&worker_cond);
  pthread_mutex_unlock (&worker_mutex);
}

void
nb_blackadder::end ()
{
  int ret;
  cerr << "ending threads..." << endl;
  pthread_mutex_lock (&selector_mutex);
  selectorShouldEnd = true;
  pthread_mutex_unlock (&selector_mutex);
  ret = write (pipe_fds[1], pipe_buf, 1);
  pthread_mutex_lock (&worker_mutex);
  workerShouldEnd = true;
  pthread_cond_signal (&worker_cond);
  pthread_mutex_unlock (&worker_mutex);
}

void *
nb_blackadder::worker (void */*arg*/)
{
  event *ev;
  while (true) {
    pthread_mutex_lock (&worker_mutex);
    pthread_cond_wait (&worker_cond, &worker_mutex);
    if (workerShouldEnd) {
      /*call cf with an end event, delete all other events and break...*/
      cout << "worker thread will now end after calling the callback with an \"end\" event" << endl;
      while (event_queue.size () > 0) {
	ev = event_queue.front ();
	event_queue.pop ();
	pthread_mutex_unlock (&worker_mutex);
	cf (ev);
	pthread_mutex_lock (&worker_mutex);
      }
      pthread_mutex_unlock (&worker_mutex);
      ev = new event ();
      ev->type = END_EVENT;
      ev->buffer = NULL;
      cf (ev);
      break;
    } else {
      while (event_queue.size () > 0) {
	ev = event_queue.front ();
	event_queue.pop ();
	pthread_mutex_unlock (&worker_mutex);
	cf (ev);
	pthread_mutex_lock (&worker_mutex);
      }
      pthread_mutex_unlock (&worker_mutex);
    }
  }
  /* Not reached unless the while-loop is terminated. */
}

void *
nb_blackadder::selector (void */*arg*/)
{
  struct msghdr msg;
  struct iovec iov;
  int total_buf_size;
  int bytes_read;
  unsigned char id_len;
  int ret;
  int high_sock;
  unsigned char *ptr = NULL;
  if (pipe_fds[0] > sock_fd) {
    high_sock = pipe_fds[0];
  } else {
    high_sock = sock_fd;
  }
  while (true) {
    if (select (high_sock + 1, &read_set, &write_set, NULL, NULL) == -1) {
      perror ("select() error..retrying!");
    } else {
      if (FD_ISSET(pipe_fds[0], &read_set)) {
	/*that's a control internal message sent in the pipe*/
	ret = read (pipe_fds[0], pipe_buf, 1);
	FD_ZERO(&write_set);
	FD_SET(sock_fd, &write_set);
      }
      pthread_mutex_lock (&selector_mutex);
      if (selectorShouldEnd) {
	pthread_mutex_unlock (&selector_mutex);
	cout << "selector thread is exiting.." << endl;
	break;
      } else {
	pthread_mutex_unlock (&selector_mutex);
      }
      if (FD_ISSET(sock_fd, &read_set)) {
	/*the netlink socket is readable*/
	memset (&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	iov.iov_base = fake_buf;
	iov.iov_len = 1;
#ifdef __linux__
	total_buf_size = recvmsg (sock_fd, &msg, MSG_PEEK | MSG_TRUNC);
#else
#ifdef __APPLE__
	socklen_t _option_len = sizeof (total_buf_size);
	if (recvmsg(sock_fd, &msg, MSG_PEEK) < 0 || getsockopt(sock_fd, SOL_SOCKET, SO_NREAD, &total_buf_size, &_option_len) < 0)
#else
	if (recvmsg(sock_fd, &msg, MSG_PEEK) < 0 || ioctl(sock_fd, FIONREAD, &total_buf_size) < 0)
#endif
	{
	  cout << "recvmsg/ioctl: " << errno << endl;
	  total_buf_size = -1;
	}
#endif

	if (total_buf_size > 0) {
	  iov.iov_base = malloc (total_buf_size);
	  iov.iov_len = total_buf_size;
	  bytes_read = recvmsg (sock_fd, &msg, 0);
	  event *ev = new event ();
	  ev->buffer = (char *) iov.iov_base;
	  ptr = (unsigned char *) ev->buffer + sizeof(struct nlmsghdr);
	  ev->type = *ptr;
	  ptr += sizeof(ev->type);
	  id_len = *ptr;
	  ptr += sizeof(id_len);
	  ev->id = string ((char *) ptr, ((int) id_len) * PURSUIT_ID_LEN);
	  ptr += ((int) id_len) * PURSUIT_ID_LEN;
	  if (ev->type == PUBLISHED_DATA) {
	    ev->data = (void *) ptr;
	    ev->data_len = bytes_read - (ptr - (unsigned char *) ev->buffer);
	  } else {
	    ev->data = NULL;
	    ev->data_len = 0;
	  }
	  pthread_mutex_lock (&worker_mutex);
	  event_queue.push (ev);
	  pthread_cond_signal (&worker_cond);
	  pthread_mutex_unlock (&worker_mutex);
	} else {
	  //perror("did not read ");
	  /*DO NOT call the callback function*/
	}
      }
      if (FD_ISSET(sock_fd, &write_set)) {
	/*the netlink socket is writable*/
	pthread_mutex_lock (&selector_mutex);
	msg = output_queue.front ();
	ret = sendmsg (sock_fd, &msg, MSG_WAITALL);
	if (ret > 0) {
	  output_queue.pop ();
	  if (msg.msg_iovlen == 2) {
	    free (msg.msg_iov[0].iov_base);
	    msg.msg_iov[0].iov_base = NULL;
	    free (msg.msg_iov[1].iov_base);
	    msg.msg_iov[1].iov_base = NULL;
	  } else {
	    free (msg.msg_iov->iov_base);
	    msg.msg_iov->iov_base = NULL;
	  }
	  free (msg.msg_iov);
	  msg.msg_iov = NULL;
	}
	//                else {
	//                    perror("could not write!!");
	//                }
	FD_ZERO(&write_set);
	if (output_queue.size () > 0) {
	  FD_ZERO(&write_set);
	  FD_SET(sock_fd, &write_set);
	} else {
	  FD_ZERO(&write_set);
	  pthread_cond_signal (&queue_overflow_cond);
	}
	pthread_mutex_unlock (&selector_mutex);
      }
      FD_ZERO(&read_set);
      FD_SET(sock_fd, &read_set);
      FD_SET(pipe_fds[0], &read_set);
    }
  }

  return NULL; /* Not reached unless the while-loop is terminated. */
}

nb_blackadder::nb_blackadder (bool user_space)
{
  int ret;
  protocol = 0;
  (void) signal (SIGINT, signal_handler);
  if (user_space) {
    cout << "Initializing blackadder client for user space" << endl;
#if HAVE_USE_NETLINK
    sock_fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
#elif HAVE_USE_UNIX
    sock_fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
#endif
  } else {
    cout << "Initializing blackadder client for kernel space" << endl;
#if HAVE_USE_NETLINK
    sock_fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_BADDER);
#elif __FreeBSD__ /* XXX */
    sock_fd = socket(AF_BLACKADDER, SOCK_RAW, PROTO_BLACKADDER);
#else
    sock_fd = -1;
    errno = EPFNOSUPPORT; /* XXX */
#endif
  }
  if (sock_fd < 0) {
    perror ("socket");
  } else {
    cout << "Created and opened netlink socket" << endl;
  }
  int x;
  x = fcntl (sock_fd, F_GETFL, 0);
  fcntl (sock_fd, F_SETFL, x | O_NONBLOCK);
#ifdef __APPLE__
  int bufsize = 229376; /* XXX */
  setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof (bufsize));
  setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof (bufsize));
#endif
  /* source address */
  memset (&s_nladdr, 0, sizeof(s_nladdr));
#if HAVE_USE_NETLINK
  s_nladdr.nl_family = AF_NETLINK;
  s_nladdr.nl_pad = 0;
  s_nladdr.nl_pid = getpid ();
  ret = bind (sock_fd, (struct sockaddr *) &s_nladdr, sizeof(s_nladdr));
#elif HAVE_USE_UNIX
  if (user_space) {
#ifndef __linux__
    s_nladdr.sun_len = sizeof (s_nladdr);
#endif
    s_nladdr.sun_family = PF_LOCAL;
    /* XXX: Probably shouldn't use getpid() here. */
    ba_id2path(s_nladdr.sun_path, getpid());
    if (unlink(s_nladdr.sun_path) != 0 && errno != ENOENT)
    perror("unlink");
#ifdef __linux__
    ret = bind(sock_fd, (struct sockaddr *) &s_nladdr, sizeof (s_nladdr));
#else
    ret = bind(sock_fd, (struct sockaddr *) &s_nladdr, SUN_LEN(&s_nladdr));
#endif
  } else {
    if (sock_fd > 0)
    ret = 0;
    else {
      ret = -1;
      errno = EBADF;
    }
  }
#endif
  if (ret < 0) {
    perror ("bind");
  }
  /* destination address */
  memset (&d_nladdr, 0, sizeof(d_nladdr));
#if HAVE_USE_NETLINK
  d_nladdr.nl_family = AF_NETLINK;
  d_nladdr.nl_pad = 0;
  if (user_space) {
    d_nladdr.nl_pid = 9999; /* destined to user space blackadder */
  } else {
    d_nladdr.nl_pid = 0; /* destined to kernel */
  }
#elif HAVE_USE_UNIX
  if (user_space) {
#ifndef __linux__
    d_nladdr.sun_len = sizeof (d_nladdr);
#endif
    d_nladdr.sun_family = PF_LOCAL;
    ba_id2path(d_nladdr.sun_path, (user_space) ? 9999 : 0); /* XXX */
  }
#endif
  /*initialize pipes*/
  if (pipe (pipe_fds) != 0) {
    perror ("pipe");
    /* XXX: Should we raise an exception or something? */
  }
  FD_ZERO(&read_set);
  FD_SET(sock_fd, &read_set);
  FD_SET(pipe_fds[0], &read_set);
  /*register default callback method*/
  cf = &default_callback;
  pthread_mutex_init (&selector_mutex, NULL);
  pthread_mutex_init (&worker_mutex, NULL);
  pthread_cond_init (&queue_overflow_cond, NULL);
  pthread_cond_init (&worker_cond, NULL);
  pthread_create (&selector_thread, NULL, selector, NULL);
  pthread_create (&worker_thread, NULL, worker, NULL);
}

nb_blackadder::~nb_blackadder ()
{
  pid_t pid = getpid ();
  if (sock_fd == -1) {
    cout << "Socket already closed" << endl;
    return;
  }
  int ret;
  struct msghdr msg;
  struct iovec iov[4];
  struct nlmsghdr _nlh, *nlh = &_nlh;
  memset (&msg, 0, sizeof(msg));
  memset (iov, 0, sizeof(iov));
  memset (nlh, 0, sizeof(*nlh));
  unsigned char type = DISCONNECT;
  /* Fill the netlink message header */
  nlh->nlmsg_len = sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type);
  nlh->nlmsg_pid = pid;
  nlh->nlmsg_flags = 1;
  nlh->nlmsg_type = 0;
  iov[0].iov_base = nlh;
  iov[0].iov_len = sizeof(*nlh);
  iov[1].iov_base = &protocol;
  iov[1].iov_len = sizeof(protocol);
  iov[2].iov_base = &pid;
  iov[2].iov_len = sizeof(pid);
  iov[3].iov_base = &type;
  iov[3].iov_len = sizeof(type);
  memset (&msg, 0, sizeof(msg));
  msg.msg_name = (void *) &d_nladdr;
  msg.msg_namelen = sizeof(d_nladdr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 4;
  ret = sendmsg (sock_fd, &msg, 0);
  if (ret < 0) {
    perror ("failed to send disconnection message");
  }
  close (sock_fd);
#if HAVE_USE_UNIX
  unlink(s_nladdr.sun_path);
#endif
  cout << "Closed netlink socket" << endl;
  sock_fd = -1;

  cout << "deleting Blackadder..." << endl;
  pthread_mutex_lock (&selector_mutex);
  if (output_queue.size () != 0) {
    pthread_cond_wait (&queue_overflow_cond, &selector_mutex);
  }
  pthread_mutex_unlock (&selector_mutex);
  pthread_cancel (worker_thread);
  pthread_cancel (selector_thread);
  if (sock_fd != -1) {
    close (sock_fd);
    cout << "Closed netlink socket" << endl;
#if HAVE_USE_UNIX
    unlink(s_nladdr.sun_path);
#endif
  }
  for (int i = 0; i < (sizeof(pipe_fds) / sizeof(pipe_fds[0])); ++i) {
    if (pipe_fds[i] != -1) close (pipe_fds[i]);
  }
}

nb_blackadder*
nb_blackadder::instance (bool user_space)
{
  if (!m_pInstance) {
    m_pInstance = new nb_blackadder (user_space);
  }
  return m_pInstance;
}

void
nb_blackadder::join ()
{
  pthread_join (selector_thread, NULL);
  pthread_join (worker_thread, NULL);
}

void
nb_blackadder::setCallback (callbacktype function)
{
  cf = function;
}

void
nb_blackadder::publish_scope (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "PUBLISH_SCOPE request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "PUBLISH_SCOPE request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "PUBLISH_SCOPE request - id cannot be empty" << endl;
  } else {
    push (PUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::publish_info (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "PUBLISH_INFO request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "PUBLISH_INFO request - wrong prefix_id size" << endl;
  } else if (prefix_id.length () == 0) {
    cout << "PUBLISH_INFO request - prefix_id cannot be empty" << endl;
  } else if (prefix_id.length () == 0) {
    cout << "PUBLISH_INFO request - prefix_id cannot be empty" << endl;
  } else {
    push (PUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::unpublish_scope (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNPUBLISH_SCOPE request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNPUBLISH_SCOPE request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "UNPUBLISH_SCOPE request - id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "UNPUBLISH_SCOPE request - id cannot consist of multiple fragments" << endl;
  } else {
    push (UNPUBLISH_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::unpublish_info (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNPUBLISH_INFO request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNPUBLISH_INFO request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "UNPUBLISH_INFO request - id cannot be empty" << endl;
  } else if (prefix_id.length () == 0) {
    cout << "UNPUBLISH_INFO request - prefix_id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "UNPUBLISH_INFO request - id cannot consist of multiple fragments" << endl;
  } else {
    push (UNPUBLISH_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::subscribe_scope (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "SUBSCRIBE_SCOPE request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "SUBSCRIBE_SCOPE request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "SUBSCRIBE_SCOPE request - id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "SUBSCRIBE_SCOPE request - id cannot consist of multiple fragments" << endl;
  } else {
    push (SUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::subscribe_info (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "SUBSCRIBE_INFO request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "SUBSCRIBE_INFO request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "SUBSCRIBE_INFO request - id cannot be empty" << endl;
  } else if (prefix_id.length () == 0) {
    cout << "SUBSCRIBE_INFO request - prefix_id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "SUBSCRIBE_INFO request - id cannot consist of multiple fragments" << endl;
  } else {
    push (SUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::unsubscribe_scope (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNSUBSCRIBE_SCOPE request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNSUBSCRIBE_SCOPE request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "UNSUBSCRIBE_SCOPE request - id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "UNSUBSCRIBE_SCOPE request - id cannot consist of multiple fragments" << endl;
  } else {
    push (UNSUBSCRIBE_SCOPE, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::unsubscribe_info (const string &id, const string &prefix_id, unsigned char strategy, void *str_opt, unsigned int str_opt_len)
{
  if (str_opt_len < 0) {
    cout << "Blackadder Library: str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNSUBSCRIBE_INFO request - wrong ID size" << endl;
  } else if (prefix_id.length () % PURSUIT_ID_LEN != 0) {
    cout << "UNSUBSCRIBE_INFO request - wrong prefix_id size" << endl;
  } else if (id.length () == 0) {
    cout << "UNSUBSCRIBE_INFO request - id cannot be empty" << endl;
  } else if (prefix_id.length () == 0) {
    cout << "UNSUBSCRIBE_INFO request - prefix_id cannot be empty" << endl;
  } else if (id.length () / PURSUIT_ID_LEN > 1) {
    cout << "UNSUBSCRIBE_INFO request - id cannot consist of multiple fragments" << endl;
  } else {
    push (UNSUBSCRIBE_INFO, id, prefix_id, strategy, str_opt, str_opt_len);
  }
}

void
nb_blackadder::push (unsigned char type, const string &id, const string &prefix_id, char strategy, void *str_opt, unsigned int str_opt_len)
{
  int ret;
  pid_t pid = getpid ();
  char *buffer;
  int buffer_length;
  struct nlmsghdr *nlh;
  struct msghdr msg;
  struct iovec *iov = (struct iovec *) calloc (1, sizeof(struct iovec));
  unsigned char id_len = id.length () / PURSUIT_ID_LEN;
  unsigned char prefix_id_len = prefix_id.length () / PURSUIT_ID_LEN;
  buffer_length = sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(prefix_id_len) + prefix_id.length () + sizeof(strategy)
      + sizeof(str_opt_len) + str_opt_len;
  buffer = (char *) malloc (buffer_length);
  nlh = (struct nlmsghdr *) buffer;
  nlh->nlmsg_len = buffer_length;
  nlh->nlmsg_pid = pid;
  nlh->nlmsg_flags = 1;
  nlh->nlmsg_type = 0;
  memcpy (buffer + sizeof(struct nlmsghdr), &protocol, sizeof(protocol));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol), &pid, sizeof(pid));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid), &type, sizeof(type));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type), &id_len, sizeof(id_len));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len), id.c_str (), id.length ());
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length (), &prefix_id_len, sizeof(prefix_id_len));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(prefix_id_len), prefix_id.c_str (), prefix_id.length ());
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(prefix_id_len) + prefix_id.length (), &strategy, sizeof(strategy));
  memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(prefix_id_len) + prefix_id.length () + sizeof(strategy),
	  &str_opt_len, sizeof(str_opt_len));
  memcpy (
      buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(prefix_id_len) + prefix_id.length () + sizeof(strategy)
	  + sizeof(str_opt_len),
      str_opt, str_opt_len);
  memset (&msg, 0, sizeof(msg));
  iov->iov_base = buffer;
  iov->iov_len = buffer_length;
  msg.msg_name = (void *) &d_nladdr;
  msg.msg_namelen = sizeof(d_nladdr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  pthread_mutex_lock (&selector_mutex);
  output_queue.push (msg);
  if (output_queue.size () == 1) {
    ret = write (pipe_fds[1], pipe_buf, 1);
  }
  if (output_queue.size () == 1000) {
    pthread_cond_wait (&queue_overflow_cond, &selector_mutex);
  }
  pthread_mutex_unlock (&selector_mutex);
}

void
nb_blackadder::publish_data (const string &id, unsigned char strategy, void *str_opt, unsigned int str_opt_len, void *a_data, unsigned int data_len)
{
  int ret;
  pid_t pid = getpid ();
  void *data = a_data;
  char *buffer;
  int buffer_length;
  struct nlmsghdr *nlh;
  struct msghdr msg;
  unsigned char type = PUBLISH_DATA;
  if (str_opt_len < 0) {
    cout << "str_opt_len must be >= 0" << endl;
  } else if (id.length () % PURSUIT_ID_LEN != 0) {
    cout << "wrong ID size" << endl;
  } else {
    struct iovec *iov = (struct iovec *) calloc (2, sizeof(struct iovec));
    unsigned char id_len = id.length () / PURSUIT_ID_LEN;
    if (iov == NULL) {
      perror ("NB_Blackadder: calloc iovecs failed");
      return;
    }
    buffer_length = sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(strategy) + sizeof(str_opt_len) + str_opt_len;
    buffer = (char *) malloc (buffer_length);
    nlh = (struct nlmsghdr *) buffer;
    nlh->nlmsg_len = buffer_length + data_len;
    nlh->nlmsg_pid = pid;
    nlh->nlmsg_flags = 1;
    nlh->nlmsg_type = 0;
    memcpy (buffer + sizeof(struct nlmsghdr), &protocol, sizeof(protocol));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol), &pid, sizeof(pid));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid), &type, sizeof(type));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type), &id_len, sizeof(id_len));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len), id.c_str (), id.length ());
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length (), &strategy, sizeof(strategy));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(strategy), &str_opt_len, sizeof(str_opt_len));
    memcpy (buffer + sizeof(struct nlmsghdr) + sizeof(protocol) + sizeof(pid) + sizeof(type) + sizeof(id_len) + id.length () + sizeof(strategy) + sizeof(str_opt_len), str_opt, str_opt_len);
    memset (&msg, 0, sizeof(msg));
    iov[0].iov_base = buffer;
    iov[0].iov_len = buffer_length;
    iov[1].iov_base = data;
    iov[1].iov_len = data_len;
    msg.msg_name = (void *) &d_nladdr;
    msg.msg_namelen = sizeof(d_nladdr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    pthread_mutex_lock (&selector_mutex);
    output_queue.push (msg);
    if (output_queue.size () == 1) {
      ret = write (pipe_fds[1], pipe_buf, 1);
    }
    if (output_queue.size () == 1000) {
      pthread_cond_wait (&queue_overflow_cond, &selector_mutex);
    }
    pthread_mutex_unlock (&selector_mutex);
  }
}
