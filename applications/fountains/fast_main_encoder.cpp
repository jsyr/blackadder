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
#include <signal.h>
#include <map>
#include <blackadder.hpp>

#include "encoder.hpp"

using namespace std;

Blackadder *ba;
pthread_t event_listener;
pthread_mutex_t event_listener_mutex = PTHREAD_MUTEX_INITIALIZER;
bool event_listener_should_exit = false;

Encoder en;

unsigned char *data;
unsigned int sizeOfData = 14000000;
unsigned short sizeOfSymbol = 1400;

map<string, pthread_t *> digital_fountain_threads;

void sigfun(int /*sig*/) {
    (void) signal(SIGINT, SIG_DFL);
    pthread_mutex_lock(&event_listener_mutex);
    event_listener_should_exit = true;
    pthread_mutex_unlock(&event_listener_mutex);
    pthread_cancel(event_listener);
    ba->disconnect();
    delete ba;
}

void *fountain_publisher(void *arg) {
    int seed;
    char *symbol, *algorithmic_identifier_buffer;
    string fountain_coding_scope(PURSUIT_ID_LEN * 2, '5'); // "5555555555555555"
    string binary_fountain_coding_scope = hex_to_chararray(fountain_coding_scope);
    string *fountain_identifier = (string *) arg;
    symbol = (char *) malloc(sizeOfSymbol);
    while (true) {
        seed = en.encodeNext(*fountain_identifier, symbol);
        algorithmic_identifier_buffer = (char *) malloc(PURSUIT_ID_LEN);
        memcpy(algorithmic_identifier_buffer, &seed, sizeof (seed));
        memcpy(algorithmic_identifier_buffer + sizeof (seed), &sizeOfData, sizeof (sizeOfData));
        string algorithmic_identifier(algorithmic_identifier_buffer, PURSUIT_ID_LEN);
        //cout << "fountain_identifier (for which rendezvous has taken place): " << chararray_to_hex(*fountain_identifier) << endl;
        //cout << "algorithmic identifier: " << chararray_to_hex(algorithmic_identifier) << endl;
        //cout << "Seed for next symbol is " << seed << endl;
        string final_binary_algorithmic_identifier = binary_fountain_coding_scope + *fountain_identifier + algorithmic_identifier;
        //cout << "identifier: " << chararray_to_hex(final_binary_algorithmic_identifier) << ", size of data: " << sizeOfData << ", seed: " << seed << endl;
        //cout << "final algorithmic identifier: " << chararray_to_hex(final_binary_algorithmic_identifier) << endl;
        ba->publish_data(final_binary_algorithmic_identifier, IMPLICIT_RENDEZVOUS_ALGID_DOMAIN, (void*) fountain_identifier->c_str(), fountain_identifier->length(), symbol, sizeOfSymbol);
    }
    delete fountain_identifier;
    free(symbol);
}

void *event_listener_loop(void *arg) {
    Blackadder *ba = (Blackadder *) arg;
    map<string, pthread_t *>::iterator digital_fountain_threads_iterator;
    pthread_t *fountain_publisher_thread;
    while (true) {
        Event ev;
        ba->getEvent(ev);
        switch (ev.type) {
            case START_PUBLISH:
                cout << "START PUBLISH: " << chararray_to_hex(ev.id) << endl;
                if (digital_fountain_threads.find(ev.id) == digital_fountain_threads.end()) {
                    fountain_publisher_thread = new pthread_t;
                    digital_fountain_threads.insert(pair<string, pthread_t *>(ev.id, fountain_publisher_thread));
                    pthread_create(fountain_publisher_thread, NULL, fountain_publisher, (void *) (new string(ev.id)));
                }
                break;
            case STOP_PUBLISH:
                cout << "STOP PUBLISH: " << chararray_to_hex(ev.id) << endl;
                digital_fountain_threads_iterator = digital_fountain_threads.find(ev.id);
                if (digital_fountain_threads_iterator != digital_fountain_threads.end()) {
                    fountain_publisher_thread = (*digital_fountain_threads_iterator).second;
                    pthread_cancel(*fountain_publisher_thread);
                    digital_fountain_threads.erase(ev.id);
                    delete fountain_publisher_thread;
                }
                break;
	    default:
		cerr << "interrupted thread or unknown event" << endl;
        }
        pthread_mutex_lock(&event_listener_mutex);
        if (event_listener_should_exit) {
            pthread_mutex_unlock(&event_listener_mutex);
            break;
        }
        pthread_mutex_unlock(&event_listener_mutex);
    }
    cout << "/*free all fountain related data*/" << endl;
    return NULL;
}

int main(int argc, char* argv[]) {
    //    /*open the file to be encoded*/
    //    ifstream toEncode;
    //    long begin;
    //    long end;
    //    toEncode.open("localrv.cc", ios::in | ios::binary);
    //    begin = toEncode.tellg();
    //    toEncode.seekg(0, ios::end);
    //    end = toEncode.tellg();
    //    sizeOfData = end - begin;
    //    cout << "size is: " << sizeOfData << " bytes.\n";
    //    /******************************************/
    /*Allocate the data to be encoded...*/
    data = (unsigned char *) malloc(sizeOfData);
    for (int i = 0; i < sizeOfData; i++) {
        data[i] = 48 + (i % 50);
    }
//    /****************DEBUG***************/
//    ofstream myfile;
//    myfile.open("before.txt");
//    for (int i = 0; i < sizeOfData; i++) {
//        myfile << data[i];
//    }
//    myfile.close();
//    cout << "done writing file" << endl;
//    /************************************/

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
    pthread_create(&event_listener, NULL, event_listener_loop, (void *) ba);
    string id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    string prefix_id = string();
    string bin_id = hex_to_chararray(id);
    string bin_prefix_id = hex_to_chararray(prefix_id);
    string full_bin_id;
    ba->publish_scope(bin_id, prefix_id, DOMAIN_LOCAL, NULL, 0);

    id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    prefix_id = string(PURSUIT_ID_LEN * 2, '1'); // "1111111111111111"
    bin_id = hex_to_chararray(id);
    bin_prefix_id = hex_to_chararray(prefix_id);
    full_bin_id = bin_prefix_id + bin_id;

    /*Initialize the encoding state*/
    //if (en.initState(full_bin_id, sizeOfData, sizeOfSymbol, NULL, &toEncode, false, time(NULL)) < 0) {
    if (en.initState(full_bin_id, sizeOfData, sizeOfSymbol, data, NULL, true) < 0) {
        cout << "an error occurred when initializing the encoding state" << endl;
    }
    cout << "initState for ID " << chararray_to_hex(full_bin_id) << ", symbol size " << sizeOfSymbol << ", number of input symbols " << sizeOfData / sizeOfSymbol << endl;
    ba->publish_info(bin_id, bin_prefix_id, DOMAIN_LOCAL, NULL, 0);

    pthread_join(event_listener, NULL);
    cout << "disconnecting" << endl;
    en.removeState(full_bin_id);
    free(data);
    return 0;
}
