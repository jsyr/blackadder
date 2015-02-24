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

#ifndef CLICK_COMMON_HH
#define CLICK_COMMON_HH

#include <click/config.h>

CLICK_DECLS

template <class T>
class PointerSetItem {
public:
    T *pointer;
    /**@brief required by Click to implement Sets using a HashTable.
     */
    typedef T * key_type;
    /**@brief required by Click to implement Sets using a HashTable.
     */
    typedef T * key_const_reference;

    /**@brief required by Click to implement Sets using a HashTable.
     */
    key_const_reference hashkey() const {
        return pointer;
    }

    /**@brief required by Click to implement Sets using a HashTable.
     */
    PointerSetItem(T * ptr) : pointer(ptr) {
    }
};

template <class T>
class ObjectSetItem {
public:
    T object;
    /**@brief required by Click to implement Sets using a HashTable.
     */
    typedef T key_type;
    /**@brief required by Click to implement Sets using a HashTable.
     */
    typedef T key_const_reference;

    /**@brief required by Click to implement Sets using a HashTable.
     */
    key_const_reference hashkey() const {
        return object;
    }

    /**@brief required by Click to implement Sets using a HashTable.
     */
    ObjectSetItem(T obj) : object(obj) {
    }
};

CLICK_ENDDECLS
#endif
