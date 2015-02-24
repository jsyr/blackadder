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
#ifndef SOLITON_DISTRIBUTION_HPP
#define SOLITON_DISTRIBUTION_HPP

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include "prng.hpp"

using namespace std;

class IdealSolitonDistribution {
public:
    IdealSolitonDistribution(unsigned int dataSize, unsigned short symbolSize, unsigned int numberOfInputSymbols);
    ~IdealSolitonDistribution();
    int getNextDegree(double randomValue);
private:
    /**the size of each symbol
     */
    unsigned short symbolSize;
    /**The size of the data to be encoded
     */
    unsigned int dataSize;
    /**the number of fragments that will be used to encode symbols
     */
    unsigned int numberOfInputSymbols;
};

class RobustSolitonDistribution {
public:
    RobustSolitonDistribution(unsigned int dataSize, unsigned short symbolSize, unsigned int numberOfInputSymbols);
    ~RobustSolitonDistribution();
    int getNextDegree(double randomValue);
    double p(unsigned int k);
    double t(unsigned int k);
    void normalise();
    unsigned int find(double randomValue, unsigned int start, unsigned int end);
private:
    /**the size of each symbol
     */
    unsigned short symbolSize;
    /**The size of the data to be encoded
     */
    unsigned int dataSize;
    /**the number of fragments that will be used to encode symbols
     */
    unsigned int numberOfInputSymbols;
    /*according to he paper*/
    double c;
    double delta;
    double R;
    double *pdf;
    double *cdf;
};

#endif