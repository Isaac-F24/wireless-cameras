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

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(remote_address.c_str());

    int status = connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address));

    if (status < 0) {
        std::cerr << "Failed to connect to server\n";
        close(sockfd);
        return -1;
    }

    while (true) {
        
        size_t length = 256;
        unsigned char* data = (unsigned char*) malloc(length);
        for (int i = 0; i < length; i++) {
            data[i] = i % 26 + 65;   // Fill with alphabet, but no RTP headers
        }

        // Send the data to the server
        unsigned long bytes_sent = send(sockfd, data, length, 0);

        free(data);

        if (bytes_sent < 0) {
            std::cerr << "Failed to send data\n";
            close(sockfd);
            return -1;
        }

        std::cout << "Sent " << bytes_sent << " bytes to server\n";
        sleep(delay_time);

    }

}