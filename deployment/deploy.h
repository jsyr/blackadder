/*
 * parser.h
 *
 *  Created on: 25 Feb 2015
 *      Author: parisis
 */

#ifndef DEPLOY_H_
#define DEPLOY_H_

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/program_options.hpp>
#include <boost/graph/graphml.hpp>

#include <boost/smart_ptr.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/property_map/property_map.hpp>

#include "network.hpp"
#include "graph_representation.hpp"
#include "parser.hpp"

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> network_graph;


#endif /* DEPLOY_H_ */
