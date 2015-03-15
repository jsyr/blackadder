all:
	$(CXX) $(CXXFLAGS) bitvector.cpp network.cpp deploy.cpp -o deploy $(LDFLAGS) -lboost_program_options -lboost_graph -lboost_filesystem -lboost_system

clean:
	rm -f deploy