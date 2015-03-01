all:
	$(CXX) $(CXXFLAGS) bitvector.cpp graph_representation.cpp parser.cpp deploy.cpp -o deploy $(LDFLAGS) -lboost_program_options -lboost_graph -lconfig++ -ligraph

clean:
	rm -f deploy