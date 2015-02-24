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
#include "soliton.hpp"

IdealSolitonDistribution::IdealSolitonDistribution(unsigned int _dataSize, unsigned short _symbolSize, unsigned int _numberOfInputSymbols) {
    dataSize = _dataSize;
    symbolSize = _symbolSize;
    numberOfInputSymbols = _numberOfInputSymbols;
}

IdealSolitonDistribution::~IdealSolitonDistribution() {

}

/*have to remove the double operations here if this is to be added in the kernel*/
int IdealSolitonDistribution::getNextDegree(double randomValue) {
    unsigned int degree = int(ceil(1 / randomValue));
    if (degree <= numberOfInputSymbols) {
        return degree;
    } else {
        return 1;
    }
    /*as described in the LT paper - above is something similar but faster*/
    //    unsigned int counter;
    //    bool done = false;
    //    if ((randomValue >= 0) && (randomValue <= ((double) 1 / (double) numberOfInputSymbols))) {
    //        degree = 1;
    //    } else {
    //        counter = 2;
    //        while (done != true) {
    //            if ((randomValue > ((double) 1 / (double) counter)) && (randomValue <= ((double) 1 / (double) (counter - 1)))) {
    //                degree = counter;
    //                done = true;
    //            } else {
    //                counter++;
    //            }
    //        }
    //    }
    return degree;
}

RobustSolitonDistribution::RobustSolitonDistribution(unsigned int _dataSize, unsigned short _symbolSize, unsigned int _numberOfInputSymbols) {
    //    cout << "Initialising Robust Soliton Distribution" << endl;
    c = 0.02;
    delta = 0.9;
    dataSize = _dataSize;
    symbolSize = _symbolSize;
    numberOfInputSymbols = _numberOfInputSymbols;
    R = c * log(numberOfInputSymbols / delta) * sqrt(numberOfInputSymbols);
    pdf = (double *) calloc(sizeof (double), numberOfInputSymbols + 1);
    cdf = (double *) calloc(sizeof (double), numberOfInputSymbols + 1);
    //    cout << setprecision(20) << "R: " << R << endl;
    pdf[0] = 0.0;
    cdf[0] = 0.0;
    //    cout << "prepared the density function tables" << endl;
    for (int i = 1; i <= numberOfInputSymbols; i++) {
        pdf[i] = p(i) + t(i);
        cdf[i] = cdf[i - 1] + pdf[i];
        //        cout << std::setprecision(20) << "pdf[" << i << "]: " << pdf[i] << endl;
        //        cout << std::setprecision(20) << "cdf[" << i << "]: " << cdf[i] << endl;
    }
    //    cout << "will normalise" << endl;
    normalise();
}

RobustSolitonDistribution::~RobustSolitonDistribution() {
    free(pdf);
    free(cdf);
}

double RobustSolitonDistribution::p(unsigned int i) {
    double output;
    if (i == 1) {
        output = (double) 1 / numberOfInputSymbols;
    } else {
        output = (double) 1 / (i * (i - 1));
    }
    //cout << setprecision(20) << "p[" << i << "]: " << output << endl;
    return output;
}

double RobustSolitonDistribution::t(unsigned int i) {
    double output;
    //    cout << "i: " << i << endl;
    //    cout << "(int) (numberOfInputSymbols / R) - 1): " << (int) (numberOfInputSymbols / R) - 1 << endl;
    //    cout << "(int) (numberOfInputSymbols / R)): " << (int) (numberOfInputSymbols / R) << endl;
    if (i <= (int) (numberOfInputSymbols / R) - 1) {
        output = (double) (R / (double) (i * numberOfInputSymbols));
    } else if (i == (int) (numberOfInputSymbols / R)) {
        output = (R * log(R / delta)) / (double) numberOfInputSymbols;
    } else {
        output = 0.0;
    }
    //cout << setprecision(20) << "t[" << i << "]: " << output << endl;
    return output;
}

void RobustSolitonDistribution::normalise() {
    double normalisation_factor = cdf[numberOfInputSymbols];
    for (int i = 1; i <= numberOfInputSymbols; i++) {
        pdf[i] = pdf[i] / normalisation_factor;
        cdf[i] = cdf[i] / normalisation_factor;
        //cout << std::setprecision(20) << "pdf[" << i << "]: " << pdf[i] << endl;
        //cout << std::setprecision(20) << "cdf[" << i << "]: " << cdf[i] << endl;
    }
}

unsigned int RobustSolitonDistribution::find(double randomValue, unsigned int start, unsigned int end) {
    unsigned int middle = start + ceil((double) (end - start) / 2.0);
    //cout << "middle: " << middle << endl;
    if (randomValue == cdf[middle] || (randomValue > cdf[middle - 1] && randomValue < cdf[middle + 1]) || middle == start) {
        //cout << "Degree: " << middle << endl;
        return middle;
    } else if (randomValue < cdf[middle]) {
        //cout << "call find again: start: " << start << ", end: " << middle - 1 << endl;
        return find(randomValue, start, middle - 1);
    } else if (randomValue > cdf[middle]) {
        //cout << "call find again: start: " << middle + 1 << ", end: " << end << endl;
        return find(randomValue, middle + 1, end);
    }
}

int RobustSolitonDistribution::getNextDegree(double randomValue) {
    //cout << "getNextDegree: " << randomValue << endl;
    return find(randomValue, 0, numberOfInputSymbols);
}
