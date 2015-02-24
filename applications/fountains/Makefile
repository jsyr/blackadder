all:
	$(CXX) $(CXXFLAGS) soliton.cpp encoder.cpp main_encoder.cpp prng.cpp -o encoder $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) soliton.cpp encoder.cpp fast_main_encoder.cpp prng.cpp -o fast_encoder $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) soliton.cpp decoder.cpp main_decoder.cpp prng.cpp -o decoder $(LDFLAGS) -lblackadder -lpthread
clean:
	rm -f encoder decoder

