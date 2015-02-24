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
#include <stdio.h>
#include <sys/time.h>
#include <fstream>
#include <signal.h>
#include <blackadder.hpp>

#include "decoder.hpp"

using namespace std;

Blackadder *ba;
Decoder decoder;
pthread_t event_listener;
pthread_mutex_t event_listener_mutex = PTHREAD_MUTEX_INITIALIZER;
bool event_listener_should_exit = false;

unsigned char *data;
unsigned short sizeOfSymbol = 1400;
unsigned int sizeOfData;

//ofstream myfile;

struct timezone tz;
struct timeval start_tv;
struct timeval end_tv;
struct timeval duration;
double throughput;
double goodput;
unsigned int counter = 0;

void sigfun(int /*sig*/) {
    (void) signal(SIGINT, SIG_DFL);
    ba->disconnect();
    delete ba;
    exit(0);
}

void *event_listener_loop(void *arg) {
    bool decodingOver;
    Event *ev;
    Blackadder *ba = (Blackadder *) arg;
    DecodingState *ds;
    string information_identifier;
    //string algorithmic_identifier;
    int seed;
    unsigned int sizeOfData = 0;
    //char *symbol;
    //cout << "event_listener_loop started " << endl;
    while (true) {
        ev = new Event();
        ba->getEvent(*ev);
        if (ev->type == PUBLISHED_DATA) {
            counter++;
            memcpy(&seed, ev->id.c_str() + ev->id.length() - PURSUIT_ID_LEN, sizeof (seed));
            memcpy(&sizeOfData, ev->id.c_str() + ev->id.length() - PURSUIT_ID_LEN + sizeof (seed), sizeof (sizeOfData));
            ds = decoder.getState(information_identifier);
            if (ds == NULL) {
                gettimeofday(&start_tv, &tz);
                ds = decoder.initState(information_identifier, sizeOfData, sizeOfSymbol, true);
            }
            decodingOver = decoder.decodeNext(ds, ev, seed);
            //cout << "identifier: " << chararray_to_hex(ev->id) << ", size of data: " << sizeOfData << ", seed: " << seed << endl;
            if (decodingOver == true) {
                //cout << "/*decoding is over*/" << endl;
                /*******************************DEBUG********************************************/
                gettimeofday(&end_tv, &tz);
                duration.tv_sec = end_tv.tv_sec - start_tv.tv_sec;
                if (end_tv.tv_usec - start_tv.tv_usec > 0) {
                    duration.tv_usec = end_tv.tv_usec - start_tv.tv_usec;
                } else {
                    duration.tv_usec = end_tv.tv_usec + 1000000 - start_tv.tv_usec;
                    duration.tv_sec--;
                }
                double left = counter * ((double) sizeOfSymbol / (double) (1024 * 1024));
                double right = ((double) ((duration.tv_sec * 1000000) + duration.tv_usec)) / 1000000;
                throughput = (left / right);
                cout << "Throughput (MB/sec): " << throughput << endl;
                left = (sizeOfData / sizeOfSymbol) * ((double) sizeOfSymbol / (double) (1024 * 1024));
                right = ((double) ((duration.tv_sec * 1000000) + duration.tv_usec)) / 1000000;
                goodput = (left / right);
                //                for (int i = 0; i < ds->numberOfInputSymbols - 1; i++) {
                //                    for (int j = 0; j < ds->symbolSize; j++) {
                //                        myfile << ((char *) ds->inputSymbols[i]->data)[j];
                //                    }
                //                }
                //                for (int j = 0; j < ds->sizeOfLastInputSymbol; j++) {
                //                    myfile << ((char *) ds->inputSymbols[ds->numberOfInputSymbols - 1]->data)[j];
                //                }
                //                myfile.close();
                /**********************************************************************************/
                
                cout << "Goodput (MB/sec): " << goodput << endl;
                cout << "Average Degree: " << ds->getAverageDegree() << endl;
                cout << "Median Degree: " << ds->getMedianDegree() << endl;
                cout << "#Symbols: " << counter << endl;
                delete ds;
                break;
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t event_listener;
    //myfile.open("after.txt");
    (void) signal(SIGINT, sigfun);
    if (argc > 1) {
        int user_or_kernel = atoi(argv[1]);
        if (user_or_kernel == 0) {
            ba = Blackadder::Instance(true);
        } else {
            ba = Blackadder::Instance(false);
        }
    } else {
        /*By Default I assume blackadder is running in user space*/
        ba = Blackadder::Instance(true);
    }
    //cout << "Process ID: " << getpid() << endl;
    string id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    string prefix_id = string();
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    ba->publish_scope(bin_id, prefix_id, DOMAIN_LOCAL, NULL, 0);

    id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    prefix_id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    ba->subscribe_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);

    string fountain_coding_scope(PURSUIT_ID_LEN * 2, '5'); // "5555555555555555"
    string binary_fountain_coding_scope = hex_to_chararray(fountain_coding_scope);
    string final_binary_algorithmic_prefix_identifier = binary_fountain_coding_scope + bin_prefix_id;

    id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    prefix_id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    ba->subscribe_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);
    ba->subscribe_info(bin_id, final_binary_algorithmic_prefix_identifier, IMPLICIT_RENDEZVOUS_ALGID_DOMAIN, NULL, 0);

    pthread_create(&event_listener, NULL, event_listener_loop, (void *) ba);

    pthread_join(event_listener, NULL);
    sleep(1);
    ba->disconnect();
    delete ba;
    return 0;
}
