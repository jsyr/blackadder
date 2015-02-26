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

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> network_graph;

namespace boost
{
  enum graph_test_t
  {
    graph_test1
  };
  enum vertex_test_t
  {
    vertex_test2
  };
  enum edge_test_t
  {
    edge_test3
  };

  BOOST_INSTALL_PROPERTY(graph, test);
  BOOST_INSTALL_PROPERTY(vertex, test);
  BOOST_INSTALL_PROPERTY(edge, test);

}
#endif /* DEPLOY_H_ */
