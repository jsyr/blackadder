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
#include <map>
#include <list>
#include <queue>
#include <vector>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <climits>

#include "soliton.hpp"

using namespace std;

class Event;
class DecodingState;
class EncodedSymbol;

class Decoder {
public:
    Decoder();
    ~Decoder();
    DecodingState *getState(string ID);
    DecodingState *initState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, bool _isMemory);
    bool decodeNext(DecodingState *ds, Event *ev, int seed);
    set<unsigned int> *getNeighbours(DecodingState *ds, unsigned int degree);
    unsigned int getNeighbour(DecodingState *ds);
    void indexEncodedSymbol(DecodingState *ds, EncodedSymbol *es);
    void decodeSymbols(DecodingState *ds);
    /*members*/
    map<string, DecodingState *> stateMap;
};

class DecodingState {
public:
    DecodingState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, bool _isMemory);
    ~DecodingState();
    double getAverageDegree();
    unsigned int getMedianDegree();
    void printQueue();
    string ID;
    /**what was given to the decoder?
     */
    bool isMemory;
    /**the size of each symbol. All symbols of a single object must have the same size. So this will be detected when the fist encoding symbol arrives.
     */
    unsigned short symbolSize;
    /**The size of the data to be encoded. This size is encoded in the identifier of each encoding symbol.
     */
    unsigned int dataSize;
    /**the number of fragments that will be used to encode symbols
     */
    unsigned int numberOfInputSymbols;
    /**the size of the last fragment (note that this can be less than the symbolSize)
     */
    unsigned short sizeOfLastInputSymbol;
    /**the number of symbols I have decoded so far
     */
    unsigned int decodedSymbols;
    /** this is an index. It maps a neighbour (an input symbol) to packets that contain this symbol (a linked list of encoded symbols)
     */
    map<unsigned int, list<EncodedSymbol *> *> neighbourIndex;
    /**this is an array of all input symbols. If an input symbol is not decoded yet, the position in the array will be NULL
     */
    Event **inputSymbols;
    vector<unsigned int> degree_statistics;
    /**This is the file where the decoded content will be stored - its name will be ID
     */
    ofstream output;
    /** queue of unprocessed symbols with degree 1 (the integer is the neighbour - the position in the array of input symbols)
     */
    queue<unsigned int> unprocessedSymbols;
    CRandomMersenne prng;
    //IdealSolitonDistribution *soliton_distribution;
    RobustSolitonDistribution *soliton_distribution;
};

class EncodedSymbol {
public:
    EncodedSymbol(Event *ev, unsigned int _degree, set<unsigned int> *_neighbours);
    ~EncodedSymbol();
    Event *_ev;
    unsigned int degree;
    set<unsigned int> *neighbours;
};

#endif