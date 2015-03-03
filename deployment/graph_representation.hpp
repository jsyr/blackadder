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

#ifndef GRAPH_REPRESENTATION_HPP
#define	GRAPH_REPRESENTATION_HPP

#include <map>
#include <igraph/igraph.h>
#include <sstream>

#include "network.h"

/* XXX: Try to figure out igraph version based on very simple assumptions. */
#define IGRAPH_V_0_6   60
#define IGRAPH_V_0_5   50
#ifdef IGRAPH_VERSION
# define IGRAPH_V IGRAPH_V_0_6
#else /* !defined(IGRAPH_VERSION) */
# define IGRAPH_V IGRAPH_V_0_5
#endif

using namespace std;

/**@brief (Deployment Application) This Class is an iGraph representation of the network domain as this was read by the configuration file.
 *
 * It is used for exporting the network domain representation to .graphML file that is later read by the Topology Manager,
 * iGraph does that automatically, so no extra effort is required.
 *
 */
class GraphRepresentation
{
public:
  /**@brief Constructor:
   *
   * @param _dm the network domain representation.
   */
  GraphRepresentation (network *_dm);
  /**@brief it builds an iGraph graph using the dm.
   *
   * @return
   */
  void
  buildIGraphTopology ();
  /**@brief It calculates a LIPSIN identifier from source node to destination node.
   *
   * It is used to calculate the default LIPSIN identifiers from each node to the domain's RV and TM.
   *
   * @param source the node label of the source node.
   * @param destination the node label of the destination node.
   * @return A Bitvector object representing the LIPSIN identifier.
   */
  bitvector
  calculateFID (string &source, string &destination);
  /**@brief calculates and assigns the default LIPSIN identifier from each node to the domain's RV.
   */
  void
  calculateRVFIDs ();
  /**@brief calculates and assigns the default LIPSIN identifier from each node to the domain's TM.
   */
  void
  calculateTMFIDs ();
  /**@brief Builds internal structures representation from an igraph instance
   *
   * It uses the igraph representation that is built from barabasi-albert algorithm and builds internal structures.
   */
  void
  buildInputMap (void);
  /**@brief builds 8-byte ascii lables from integer ids
   *
   * builds 8-byte ascii lables from integer ids
   *
   * @param id the integer identifier for conversion
   * @return the string representation for the identifier
   */
  string
  produceStringLabelsFromIds (int id);
  /**@brief Find the leaf vertices of the graph.
   * This function finds the leaf vertices of the current graph and outputs the result to a file in a specific format. This file can be used for experiments.
   *
   * @param filename the filename for the output.
   */
  void
  outputLeafVertices (string filename);
  /**@brief Chooses the best TM RV node by finding the node with the least distance from the rest.
   *
   * Chooses the best TM RV node by finding the node with the least distance from the rest. It sets the RV TM in the internal deployment tool graph representation
   */
  void
  chooseBestTMRVNode (void);
  /**@brief adds  a connection in the internal deployment tool representation between the vertices that are provided as input
   *
   * adds  a connection in the internal deployment tool representation between the vertices that are provided as input
   *
   * @param nn1 network vertex 1
   * @param nn2 network vertex 2
   */
  void
  addGeneratedMappedConnection (network_node *nn1, network_node *nn2);

  /**
   * @brief node_index another STL map used for planetlab node indexing
   */
  map<int, string> node_index;
  /**
   * @brief the iGraph graph.
   */
  igraph_t igraph;
  /**
   * @brief a map of node labels to iGraph vertex ids.
   */
  map<string, int> reverse_node_index;
  /**
   * @brief the network domain as read by the provided configuration file.
   */
  network *dm;
};

#endif
