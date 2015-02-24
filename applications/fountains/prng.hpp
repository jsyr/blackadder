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

#ifndef PRNG_HPP
#define PRNG_HPP

#include <click/config.h>

CLICK_DECLS

//typedef signed int int32_t;
//typedef unsigned int uint32_t;
//typedef long long int64_t;
//typedef unsigned long long uint64_t;

class CRandomMersenne {
    // Choose which version of Mersenne Twister you want:
#if 0 
    // Define constants for type MT11213A:
#define MERS_N   351
#define MERS_M   175
#define MERS_R   19
#define MERS_U   11
#define MERS_S   7
#define MERS_T   15
#define MERS_L   17
#define MERS_A   0xE4BD75F5
#define MERS_B   0x655E5280
#define MERS_C   0xFFD58000
#else    
    // or constants for type MT19937:
#define MERS_N   624
#define MERS_M   397
#define MERS_R   31
#define MERS_U   11
#define MERS_S   7
#define MERS_T   15
#define MERS_L   18
#define MERS_A   0x9908B0DF
#define MERS_B   0x9D2C5680
#define MERS_C   0xEFC60000
#endif

public:

    CRandomMersenne(int seed) {
        RandomInit(seed);
        LastInterval = 0;
    };
    void RandomInit(int seed); // Re-seed
    void RandomInitByArray(int const seeds[], int NumSeeds); // Seed by more than 32 bits
    int IRandomX(int min, int max); // Output random integer, exact
    uint32_t BRandom(); // Output random bits
private:
    void Init0(int seed); // Basic initialization procedure
    uint32_t mt[MERS_N]; // State vector
    int mti; // Index into mt
    uint32_t LastInterval; // Last interval length for IRandomX
    uint32_t RLimit; // Rejection limit used by IRandomX
};

class CRandomMother { // Encapsulate random number generator
public:

    CRandomMother(int seed) { // Constructor
        RandomInit(seed);
    };
    void RandomInit(int seed); // Initialization
    int IRandom(int min, int max); // Get integer random number in desired interval
    //double Random(); // Get floating point random number
    uint32_t BRandom(); // Output random bits
protected:
    uint32_t x[5]; // History buffer
};

CLICK_ENDDECLS

#endif
