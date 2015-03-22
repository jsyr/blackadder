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
#include "decoder.h"

#include <stdio.h>
#include <blackadder.h>

Decoder::Decoder() {
}

Decoder::~Decoder() {
}

DecodingState *Decoder::getState(string ID) {
    map<string, DecodingState *>::iterator it;
    it = stateMap.find(ID);
    if (it == stateMap.end()) {
        return NULL;
    } else {
        return (*it).second;
    }
}

DecodingState *Decoder::initState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, bool _isMemory) {
    DecodingState *ds = new DecodingState(_ID, _dataSize, _symbolSize, _isMemory);
    stateMap.insert(pair<string, DecodingState *> (_ID, ds));
    return ds;
}

bool Decoder::decodeNext(DecodingState *ds, event *ev, int seed) {
    EncodedSymbol *es;
    int degree;
    long *test1;
    long *test2;
    set<unsigned int> *neighbours;
    set<unsigned int>::iterator neighboursIt;
    unsigned int neighbourIndex;
    ds->prng.RandomInit(seed);
    degree = ds->soliton_distribution->getNextDegree((double) ds->prng.IRandomX(0, INT_MAX) / INT_MAX);
    ds->degree_statistics.push_back(degree);
    //cout << "symbol contains " << degree << " neighbours " << endl;
    //cout << "Received a new symbol to decode - ID: " << ds->ID << ", degree: " << degree << ", seed: " << seed << ", dataSize: " << ds->dataSize << ", symbolSize: " << ds->symbolSize << endl;
    if (degree > 1) {
        /*use the seed to calculate all neighbours of this encoding symbol*/
        neighbours = getNeighbours(ds, degree);
        /*first, for each neighbour check if the respective symbol is decoded*/
        /*In that case, remove the neighbour from the set..also remove the respective symbol by XORing*/
        /*if all neighbours are decoded, discard the symbol - it's useless*/
        neighboursIt = neighbours->begin();
        while (neighboursIt != neighbours->end()) {
            neighbourIndex = *neighboursIt;
            //cout << "checking " << neighbourIndex << endl;
            if (ds->inputSymbols[neighbourIndex] != NULL) {
                //cout << "removing neighbour " << *neighboursIt << endl;
                neighbours->erase(neighboursIt++);
                degree--;
                /*XOR the encoding symbol with the input symbol*/
                for (unsigned short j = 0; j < ds->symbolSize / sizeof (long); j++) {
                    test1 = (long *) ev->data;
                    test2 = (long *) ds->inputSymbols[neighbourIndex]->data;
                    test1[j] = test1[j]^test2[j];
                }
            } else {
                neighboursIt++;
            }
        }
        if (neighbours->size() == 0) {
            delete neighbours;
            delete ev;
            return false;
        } else if (neighbours->size() == 1) {
            if (ds->inputSymbols[*neighbours->begin()] != NULL) {
                //cout << "symbol is already decoded...discarding" << endl;
                delete neighbours;
                delete ev;
                return false;
            } else {
                ds->inputSymbols[*neighbours->begin()] = ev;
                ds->decodedSymbols++;
                ds->unprocessedSymbols.push(*neighbours->begin());
                //cout << "3) added to unprocessed " << *neighbours->begin() << endl;
                delete neighbours;
            }
        } else {
            /*create and store the encoding symbol using the (remaining) neighbours as indices*/
            es = new EncodedSymbol(ev, degree, neighbours);
            indexEncodedSymbol(ds, es);
        }
    } else {
        /*the degree is one - use the seed to calculate the input symbol number and store it in the inputSymbols array for this information item*/
        neighbourIndex = getNeighbour(ds);
        //cout << "received symbol with degree = 1. neighbour is:" << neighbourIndex << endl;
        /*check if the symbol is already encoded and discard*/
        if (ds->inputSymbols[neighbourIndex] != NULL) {
            //cout << "symbol is already decoded...discarding" << endl;
            delete ev;
            return false;
        } else {
            ds->inputSymbols[neighbourIndex] = ev;
            ds->decodedSymbols++;
            ds->unprocessedSymbols.push(neighbourIndex);
            //cout << "1) added to unprocessed " << neighbourIndex << endl;
        }
    }
    while (!ds->unprocessedSymbols.empty()) {
        decodeSymbols(ds);
    }
    if (ds->decodedSymbols == ds->numberOfInputSymbols) {
        return true;
    }
    return false;
}

void Decoder::decodeSymbols(DecodingState *ds) {
    char *symbol;
    long *test1;
    long *test2;
    unsigned int neighbour;
    unsigned int neighbourToAdd;
    /*this is an iterator to the index stored for this information item*/
    map<unsigned int, list<EncodedSymbol *> *>::iterator indexIt;
    map<unsigned int, list<EncodedSymbol *> *>::iterator deleteIndexIt;
    /*this is the list of encoding symbols for a specific neighbour*/
    list<EncodedSymbol *> * symbolList;
    list<EncodedSymbol *> * deleteSymbolList;
    /*an iterator to the above list*/
    list<EncodedSymbol *>::iterator symbolListIt;
    list<EncodedSymbol *>::iterator deleteSymbolListIt;
    EncodedSymbol *es;
    unsigned int counter = 0;
    /*get the next packet from the queue of unprocessed packets*/
    neighbour = ds->unprocessedSymbols.front();
    ds->unprocessedSymbols.pop();
    symbol = (char *) ds->inputSymbols[neighbour]->data;
    /***********************************************************/
    //cout << "decodeSymbols: " << neighbour << endl;
    /*find an entry in our index for this neighbour (the one for which the packet is decoded)*/
    indexIt = ds->neighbourIndex.find(neighbour);
    if (indexIt != ds->neighbourIndex.end()) {
        /*for each encoded symbol indexed under this neighbour XOR the symbol and delete this neighbour (also delete it from the list).
         *If this was its last occurrence, add the XORed result to the input symbols array and call the method recursively.
         */
        symbolList = (*indexIt).second;
        symbolListIt = symbolList->begin();
        while (symbolListIt != symbolList->end()) {
            counter++;
            es = (*symbolListIt);
            //cout << "neighbour list size: " << es->neighbours->size() << endl;
            es->neighbours->erase(neighbour);
            /*decrease the degree for this encoding symbol - it has to reach the value of 1 to be decoded*/
            es->degree--;
            /*XOR the encoding symbol with the input symbol*/
            for (unsigned short j = 0; j < ds->symbolSize / sizeof (long); j++) {
                test1 = (long *) es->_ev->data;
                test2 = (long *) symbol;
                test1[j] = test1[j]^test2[j];
            }
            /*delete the encoding symbol from the index*/
            symbolListIt = symbolList->erase(symbolListIt);
            if (es->degree == 1) {
                neighbourToAdd = *es->neighbours->begin();
                /*check if this symbol has already been decoded (this usually happens as a chain reaction)*/
                if (ds->inputSymbols[neighbourToAdd] == NULL) {
                    //cout << "2) added to unprocessed " << neighbourToAdd << endl;
                    ds->inputSymbols[neighbourToAdd] = es->_ev;
                    ds->decodedSymbols++;
                    ds->unprocessedSymbols.push(neighbourToAdd);
                    /*The symbol should also exist in the respective index of its neighbour...delete it from there and delete the object*/
                    deleteIndexIt = ds->neighbourIndex.find(neighbourToAdd);
                    deleteSymbolList = (*deleteIndexIt).second;
                    deleteSymbolListIt = deleteSymbolList->begin();
                    while (deleteSymbolListIt != deleteSymbolList->end()) {
                        if (*deleteSymbolListIt == es) {
                            deleteSymbolList->erase(deleteSymbolListIt);
                            break;
                        }
                        deleteSymbolListIt++;
                    }
                    delete es;
                } else {
                    //cout << "coincidence...this was already decoded" << endl;
                    /*The symbol should also exist in the respective index of its neighbour...delete it from there and delete the object*/
                    deleteIndexIt = ds->neighbourIndex.find(neighbourToAdd);
                    deleteSymbolList = (*deleteIndexIt).second;
                    deleteSymbolListIt = deleteSymbolList->begin();
                    while (deleteSymbolListIt != deleteSymbolList->end()) {
                        if (*deleteSymbolListIt == es) {
                            deleteSymbolList->erase(deleteSymbolListIt);
                            break;
                        }
                        deleteSymbolListIt++;
                    }
                    delete es->_ev;
                    delete es;
                }
            }
        }
        /*delete the index for this neighbour if there are no other encoding symbols*/
        //cout << "delete the list of symbols and the index for neighbour " << neighbour << endl;
        delete symbolList;
        ds->neighbourIndex.erase(indexIt);
    } else {
        //cout << "nothing to be done! There is no encoded symbol with neighbour " << neighbour << endl;
    }
}

set<unsigned int> *Decoder::getNeighbours(DecodingState *ds, unsigned int degree) {
    unsigned int neighbourIndex;
    set<unsigned int> *neighbours = new set<unsigned int>();
    //cout << "get neighbours" << endl;
    for (unsigned int i = 0; i < degree; i++) {
        neighbourIndex = ds->prng.IRandomX(0, ds->numberOfInputSymbols - 1);
        //cout << "neighbour " << neighbourIndex << endl;
        if (neighbours->find(neighbourIndex) != neighbours->end()) {
            //cout << "a duplicate when decoding " << neighbourIndex << endl;
            i--;
            continue;
        }
        neighbours->insert(neighbourIndex);
    }
    return neighbours;
}

unsigned int Decoder::getNeighbour(DecodingState *ds) {
    unsigned int neighbourIndex;
    neighbourIndex = ds->prng.IRandomX(0, ds->numberOfInputSymbols - 1);
    //cout << "get neighbour" << neighbourIndex << endl;
    return neighbourIndex;
}

void Decoder::indexEncodedSymbol(DecodingState *ds, EncodedSymbol *es) {
    map<unsigned int, list<EncodedSymbol *> *>::iterator it;
    set<unsigned int>::iterator setIt = es->neighbours->begin();
    for (unsigned int i = 0; i < es->degree; i++) {
        it = ds->neighbourIndex.find(*setIt);
        if (it == ds->neighbourIndex.end()) {
            list<EncodedSymbol *> *symbolList = new list<EncodedSymbol *>();
            symbolList->push_back(es);
            //cout << "added neighbour " << *setIt << " in the index." << " List size: " << symbolList->size() << endl;
            ds->neighbourIndex.insert(pair<unsigned int, list<EncodedSymbol *> *>(*setIt, symbolList));
        } else {
            //cout << "/*it is not the first time an encoding symbol contains this neighbour*/" << endl;
            (*it).second->push_back(es);
        }
        //cout << "size of index: " << ds->neighbourIndex.size() << endl;
        setIt++;
    }
}

DecodingState::DecodingState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, bool _isMemory) : prng(0) {
    ID = _ID;
    dataSize = _dataSize;
    symbolSize = _symbolSize;
    isMemory = _isMemory;
    if (_dataSize % _symbolSize == 0) {
        numberOfInputSymbols = _dataSize / _symbolSize;
        sizeOfLastInputSymbol = _symbolSize;
    } else {
        numberOfInputSymbols = (_dataSize / _symbolSize) + 1;
        sizeOfLastInputSymbol = _dataSize % _symbolSize;
    }
    if (isMemory) {
        inputSymbols = (event **) calloc(sizeof (event *), numberOfInputSymbols);
    } else {
        /*XXX not ready yet*/
        output.open(ID.c_str(), ios::out | ios::binary);
    }
    decodedSymbols = 0;
    //soliton_distribution = new IdealSolitonDistribution(dataSize, symbolSize, numberOfInputSymbols);
    soliton_distribution = new RobustSolitonDistribution(dataSize, symbolSize, numberOfInputSymbols);
}

DecodingState::~DecodingState() {
    //map<unsigned int, list<EncodedSymbol *> *>::iterator indexIt;
    delete soliton_distribution;
    for (unsigned int i = 0; i < numberOfInputSymbols; i++) {
        delete(inputSymbols[i]);
    }
    free(inputSymbols);
}

double DecodingState::getAverageDegree() {
    vector<unsigned int>::iterator it;
    unsigned int sum = 0;
    for (it = degree_statistics.begin(); it != degree_statistics.end(); it++) {
        sum += (*it);
    }
    return ((double) sum / degree_statistics.size());
}

unsigned int DecodingState::getMedianDegree() {
    return degree_statistics[degree_statistics.size()/2];
}

EncodedSymbol::EncodedSymbol(event *ev, unsigned int _degree, set<unsigned int> *_neighbours) {
    _ev = ev;
    degree = _degree;
    neighbours = _neighbours;
}

EncodedSymbol::~EncodedSymbol() {
    /*I will not free the data - the buffer will be used later*/
    neighbours->clear();
    delete neighbours;
}
