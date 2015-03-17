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

#ifndef PARAMS_H_
#define PARAMS_H_

#include <iostream>

/* name of configuration file */
extern std::string conf;

/* a global variable: the folder where all configuration files will be stored before deploying blackadder */
extern std::string output_folder;

/* format of configuration file ("xml","json","ini","info") - default: "xml" */
extern std::string format;

extern bool no_discover;
extern bool no_copy;
extern bool no_start;
extern bool verbose;

#endif /* PARAMS_H_ */
