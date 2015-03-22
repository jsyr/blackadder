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

#ifndef DECODER_HPP
#define DECODER_HPP

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <map>
#include <fstream>
#include <climits>

#include "soliton.h"

using namespace std;

class EncodingState;

/**
 * Encoder keeps some state about the objects that need to be encoded..each time it creates a new encoding symbol for the specific object.
 * It can create encoding symbols for a data allocated and passed as a const char *, or for data stored in a file using the provided file descriptor.
 */
class Encoder {
public:
    Encoder();
    ~Encoder();
    int initState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, unsigned char *_data, ifstream *toEncode, bool _isMemory);
    int removeState(string ID);
    /**This method will calculate the next encoded symbol for the provided file descriptor.
     * Memory space for the encoded symbol must be already allocated with the appropriate symbol size.
     */
    int encodeNext(string &ID, char *symbol);
    map<string, EncodingState *> stateMap;
    CRandomMersenne seed_generator;
};

class EncodingState {
public:
    EncodingState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, unsigned char *_data, ifstream *_toEncode, bool _isMemory);
    ~EncodingState();
    string ID;
    /**the data over which we encode (may be null if a file descriptor was given)
     */
    unsigned char *data;
    /**the file descriptor over which we encode (may be -1 if a memory space was given)
     */
    ifstream *toEncode;
    /**what was given to the encoder?
     */
    bool isMemory;
    /**the size of each symbol
     */
    unsigned short symbolSize;
    /**The size of the data to be encoded
     */
    unsigned int dataSize;
    /**the number of fragments that will be used to encode symbols
     */
    unsigned int numberOfInputSymbols;
    /**the size of the last fragment (note that this can be less than the symbolSize)
     */
    unsigned short sizeOfLastInputSymbol;
    /**The last input symbol will always be on memory
     */
    char *lastInputSymbol;
    /**this set is used each time a symbol is encoded so that duplicate input symbols are avoided - it is cleared after each encoding call
     */
    set<int> neighbours;
    //IdealSolitonDistribution *soliton_distribution;
    RobustSolitonDistribution *soliton_distribution;
    CRandomMersenne prng;
};

#endif
