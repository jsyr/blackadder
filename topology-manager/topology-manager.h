/*
 * topology-manager.h
 *
 *  Created on: 17 Mar 2015
 *      Author: parisis
 */

#ifndef TOPOLOGY_MANAGER_H_
#define TOPOLOGY_MANAGER_H_

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include <signal.h>
#include <blackadder.h>

#include "tm_graph.h"

typedef std::pair<std::string, boost::shared_ptr<bitvector> > result_map_iter;

#endif /* TOPOLOGY_MANAGER_H_ */
