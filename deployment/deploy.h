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

#include "network.hpp"
#include "graph_representation.hpp"
#include "parser.hpp"

struct graph_properties
{
  int test_graph;
};

struct vertex_properties
{
  int test_vertex;
};

struct edge_properties
{
  int test_edge;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, vertex_properties, edge_properties, graph_properties> graph;

#endif /* DEPLOY_H_ */
