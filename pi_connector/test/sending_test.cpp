#include <uvgrtp/lib.hh>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

const std::string remote_address = "127.0.0.1";

// Tests the connecter by sending a 256x256 frame of random data to a chosen port using RTP.
int main(int argc, char** argv) {

    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> [<delay time (s)>]\n";
        return -1;
    }
    int port = std::stoi(argv[1]);
    if (port < 1024 || port > 65535) {
        std::cerr << "Port number must be between 1024 and 65535\n";
        return -1;
    }

    int delay_time = 1;
    if (argc == 3) {
        delay_time = std::stoi(argv[2]);
        if (delay_time <= 0) {
            std::cerr << "Delay time must be a positive integer\n";
            return -1;
        }
    }

    uvgrtp::context context;
    uvgrtp::session* session = context.create_session(remote_address);
    if (session == NULL) {
        std::cerr << "Failed to create session\n";
        return -1;
    }

    uvgrtp::media_stream* stream = session->create_stream(port, RTP_FORMAT_GENERIC, RCE_SEND_ONLY | RCE_FRAGMENT_GENERIC);
    if (stream == NULL) {
        std::cerr << "Failed to create stream\n";
        return -1;
    }

    while (true) {
        
        size_t length = 256 * 256;
        unsigned char* data = (unsigned char*) malloc(length);

        for (int i = 0; i < length; i++) {
            data[i] = i % 26 + 65;   // Fill with alphabet :)
        }

        rtp_error_t status = stream->push_frame(data, length, RTP_NO_FLAGS);

        free(data);

        if (status != RTP_OK) {
            std::cerr << "Failed to push frame: " << status << "\n";
            return -1;
        }

        sleep(delay_time);
        std::cout << "Sent frame\n";
    }

}