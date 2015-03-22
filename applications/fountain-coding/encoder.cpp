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
#include "encoder.h"

Encoder::Encoder() : seed_generator(time(NULL)) {

}

Encoder::~Encoder() {

}

int Encoder::initState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, unsigned char *_data, ifstream *_toEncode, bool _isMemory) {
    EncodingState *state;
    map<string, EncodingState *>::iterator it;
    it = stateMap.find(_ID);
    if (it == stateMap.end()) {
        state = new EncodingState(_ID, _dataSize, _symbolSize, _data, _toEncode, _isMemory);
        stateMap.insert(pair<string, EncodingState *> (_ID, state));
    } else {
        cout << "/*this identifier exists..return an error*/" << endl;
        return -1;
    }
    return 0;
}

int Encoder::removeState(string ID) {
    map<string, EncodingState *>::iterator it = stateMap.find(ID);
    if (it != stateMap.end()) {
        delete (*it).second;
        stateMap.erase(ID);
        return 0;
    } else {
        return -1;
    }
}

int Encoder::encodeNext(string &ID, char *symbol) {
    int seed;
    int degree;
    int neighbourIndex = 0;
    long *test1;
    long *test2;
    EncodingState *es;
    map<string, EncodingState *>::iterator it;
    it = stateMap.find(ID);
    if (it != stateMap.end()) {
        seed = seed_generator.IRandomX(INT_MIN + 1, INT_MAX);
        //cout << "Seed: " << seed << endl;
        es = (*it).second;
        es->prng.RandomInit(seed);
        degree = es->soliton_distribution->getNextDegree((double) es->prng.IRandomX(0, INT_MAX) / INT_MAX);
        //cout << "symbol contains " << degree << " neighbours " << endl;
        for (unsigned int i = 0; i < degree; i++) {
            neighbourIndex = es->prng.IRandomX(0, es->numberOfInputSymbols - 1);
            //cout << "neighbour " << neighbourIndex << endl;
            if (es->neighbours.find(neighbourIndex) != es->neighbours.end()) {
                //cout << "a duplicate when encoding " << neighbourIndex << endl;
                i--;
                continue;
            }
            es->neighbours.insert(neighbourIndex);
            if (i == 0) {
                if (neighbourIndex == es->numberOfInputSymbols - 1) {
                    /*be careful - this is the last input symbol*/
                    memcpy(symbol, es->lastInputSymbol, es->symbolSize);
                } else {
                    if (es->data != NULL) {
                        memcpy(symbol, es->data + neighbourIndex * (es->symbolSize), es->symbolSize);
                    } else {
                        /*seek the file to the right input symbol*/
                        es->toEncode->seekg(neighbourIndex * (es->symbolSize));
                        es->toEncode->read(symbol, es->symbolSize);
                    }
                }
            } else {
                test1 = (long *) symbol;
                if (neighbourIndex == es->numberOfInputSymbols - 1) {
                    /*be careful - this is the last input symbol*/
                    test2 = (long *) (es->lastInputSymbol);
                    for (int j = 0; j < (es->symbolSize) / sizeof (long); j++) {
                        test1[j] ^= test2[j];
                    }
                } else {
                    if (es->data != NULL) {
                        test2 = (long *) (es->data);
                        for (int j = 0; j < (es->symbolSize) / sizeof (long); j++) {
                            test1[j] ^= test2[(neighbourIndex * (es->symbolSize)) / sizeof (long) +j];
                        }
                    } else {
                        /*allocate some memory and seek the file to the right input symbol*/
                        char *memory_space = (char *) malloc((es->symbolSize));
                        es->toEncode->seekg(neighbourIndex * (es->symbolSize));
                        es->toEncode->read(memory_space, es->symbolSize);
                        test2 = (long *) (memory_space);
                        for (int j = 0; j < (es->symbolSize) / sizeof (long); j++) {
                            test1[j] ^= test2[j];
                        }
                        free(memory_space);
                    }
                }
            }
        }
        es->neighbours.clear();
        return seed;
    } else {
        cout << "there is no encoding state for this identifier" << endl;
        return -1;
    }
    return 0;
}

EncodingState::EncodingState(string _ID, unsigned int _dataSize, unsigned short _symbolSize, unsigned char *_data, ifstream *_toEncode, bool _isMemory) : prng(0) {
    ID = _ID;
    dataSize = _dataSize;
    symbolSize = _symbolSize;
    data = _data;
    toEncode = _toEncode;
    isMemory = _isMemory;
    lastInputSymbol = (char *) malloc(symbolSize);
    memset(lastInputSymbol, 0, symbolSize);
    if (dataSize % symbolSize != 0) {
        numberOfInputSymbols = (dataSize / symbolSize) + 1;
        sizeOfLastInputSymbol = dataSize % symbolSize;
    } else {
        numberOfInputSymbols = dataSize / symbolSize;
        sizeOfLastInputSymbol = symbolSize;
    }
    if (data != NULL) {
        memcpy(lastInputSymbol, data + (numberOfInputSymbols - 1) * symbolSize, sizeOfLastInputSymbol);
    } else {
        /*seek the file to the last input symbol*/
        toEncode->seekg((numberOfInputSymbols - 1) * symbolSize);
        toEncode->read(lastInputSymbol, sizeOfLastInputSymbol);
    }
    //soliton_distribution = new IdealSolitonDistribution(dataSize, symbolSize, numberOfInputSymbols);
    soliton_distribution = new RobustSolitonDistribution(dataSize, symbolSize, numberOfInputSymbols);
}

EncodingState::~EncodingState() {
    delete soliton_distribution;
    free(lastInputSymbol);
}
