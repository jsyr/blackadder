#!/usr/bin/env python

# Copyright (C) 2012, Oy L M Ericsson Ab, NomadicLab. All rights reserved.

import igraph

def _main(argv):
    topo_file = "topology.graphml" if len(argv) < 2 else argv[1]
    gfx_file = "graph.svg" if len(argv) < 3 else argv[2]

    g = igraph.read(topo_file)

    l = "lgl"
#    l = g.layout_reingold_tilford(0)
    w, h = 800, 800
    size = 14
    g.write_svg(gfx_file, layout=l, width=w, height=h,
                vertex_size=size, font_size=size,
                labels=[v['id'][1:] for v in g.vs],
                colors=["lightblue"]*len(g.vs), edge_colors=["grey"]*len(g.es))

if __name__ == "__main__":
    import sys
    _main(sys.argv)
