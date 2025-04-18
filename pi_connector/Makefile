all: connector send_test receive_test bogus_test

connector: connector.cpp
	g++ connector.cpp -g -luvgrtp -lpthread -o connector

send_test: test/sending_test.cpp
	g++ test/sending_test.cpp -g -luvgrtp -lpthread -o test/sending_test

receive_test: test/receiving_test.cpp
	g++ test/receiving_test.cpp -g -luvgrtp -lpthread -o test/receiving_test

bogus_test: test/bogus_test.cpp
	g++ test/bogus_test.cpp -g -luvgrtp -lpthread -o test/bogus_test

# see uvgRTP/BUILDING.md for building/installing uvgRTP