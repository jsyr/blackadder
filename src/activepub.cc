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
#include "activepub.hh"

CLICK_DECLS

ActivePublication::ActivePublication(String _fullID, unsigned char _strategy, const void *_str_opt, unsigned int _str_opt_len, bool _isScope) {
    fullID = _fullID;
    strategy = _strategy;
    isScope = _isScope;
    subsFID = NULL;
    subsExist = false;
    str_opt_len = _str_opt_len;
    str_opt = NULL;
    if (str_opt_len > 0) {
        str_opt = CLICK_LALLOC(str_opt_len);
        memcpy(str_opt, _str_opt, str_opt_len);
    }
}

ActivePublication::~ActivePublication() { 
    if (subsFID != NULL) {
        delete subsFID;
    }
    if (str_opt_len > 0) {
        CLICK_LFREE(str_opt, str_opt_len);
    }
}

ActiveSubscription::ActiveSubscription(String _fullID, unsigned char _strategy, const void *_str_opt, unsigned int _str_opt_len, bool _isScope) {
    fullID = _fullID;
    strategy = _strategy;
    isScope = _isScope;
    str_opt_len = _str_opt_len;
    str_opt = NULL;
    if (str_opt_len > 0) {
        str_opt = CLICK_LALLOC(str_opt_len);
        memcpy(str_opt, _str_opt, str_opt_len);
    }
}

ActiveSubscription::~ActiveSubscription() {
    if (str_opt_len > 0) {
        CLICK_LFREE(str_opt, str_opt_len);
    }
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(ActivePublication)
ELEMENT_PROVIDES(ActiveSubscription)
