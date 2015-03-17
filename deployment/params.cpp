/*
 * Copyright (C) 2015  George Parisis
 * All rights reserved.
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of
 * the BSD license.
 *
 * See LICENSE and COPYING for more details.
 */

#include "params.h"

/* name of configuration file */
std::string conf;

/* a global variable: the folder where all configuration files will be stored before deploying blackadder */
std::string output_folder = "/tmp";

/* format of configuration file ("xml","json","ini","info") - default: "xml" */
std::string format = "xml";

bool no_discover = false;
bool no_copy = false;
bool no_start = false;
bool verbose = false;
