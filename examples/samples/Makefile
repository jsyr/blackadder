all:
	$(CXX) $(CXXFLAGS) channel_publisher.cpp -o channel_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) channel_subscriber.cpp -o channel_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) publisher.cpp  -o publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) subscriber.cpp -o subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) nb_publisher.cpp  -o nb_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) nb_subscriber.cpp -o nb_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) broadcast_subscriber.cpp -o broadcast_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) broadcast_publisher.cpp -o broadcast_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) algid_publisher.cpp -o algid_publisher $(LDFLAGS) -lblackadder -lpthread -lcrypto
	$(CXX) $(CXXFLAGS) algid_subscriber.cpp -o algid_subscriber $(LDFLAGS) -lblackadder -lpthread -lcrypto
	$(CXX) $(CXXFLAGS) nb_channel_publisher.cpp -o nb_channel_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) nb_channel_subscriber.cpp -o nb_channel_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) link_local_publisher.cpp -o link_local_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) link_local_subscriber.cpp -o link_local_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) implicit_rendezvous_publisher.cpp -o implicit_rendezvous_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) implicit_rendezvous_subscriber.cpp -o implicit_rendezvous_subscriber $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) implicit_algid_rendezvous_publisher.cpp -o implicit_algid_rendezvous_publisher $(LDFLAGS) -lblackadder -lpthread
	$(CXX) $(CXXFLAGS) implicit_algid_rendezvous_subscriber.cpp -o implicit_algid_rendezvous_subscriber $(LDFLAGS) -lblackadder -lpthread
clean:
	rm -f implicit_algid_rendezvous_publisher implicit_algid_rendezvous_subscriber implicit_rendezvous_publisher implicit_rendezvous_subscriber channel_publisher channel_subscriber publisher subscriber nb_publisher nb_subscriber broadcast_publisher broadcast_subscriber algid_subscriber algid_publisher nb_channel_publisher nb_channel_subscriber link_local_publisher link_local_subscriber
