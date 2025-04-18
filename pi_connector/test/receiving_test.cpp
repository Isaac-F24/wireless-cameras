#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>

const std::string socket_path = "/tmp/wireless_camera_socket";

int main(int argc, char** argv) {

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    // remove the socket file if it already exists
    unlink(socket_path.c_str());

    int status = bind(socket_fd, (struct sockaddr*)&address, sizeof(address));
    if (status < 0) {
        std::cerr << "Failed to bind socket\n";
        close(socket_fd);
        return -1;
    }

    status = listen(socket_fd, 1);
    if (status < 0) {
        std::cerr << "Failed to listen on socket\n";
        close(socket_fd);
        return -1;
    }

    std::cout << "Waiting for connection on socket at " << socket_path << "\n";

    int client_fd = accept(socket_fd, NULL, NULL);
    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        close(socket_fd);
        return -1;
    }

    std::cout << "Connected to client!\n";

    while (true) {
        long camID;
        long size;

        // read the camID and size from the socket
        long bytes_read = read(client_fd, &camID, sizeof(camID));
        bytes_read += read(client_fd, &size, sizeof(size));

        std::cout << "Received frame from camera " << camID << "\n";
        std::cout << "Size: " << size << "\n";

        if (bytes_read != sizeof(camID) + sizeof(size)) {
            std::cerr << "Failed to read id/size\n";
            break;
        }

        std::vector<unsigned char> frame(size);
        const char* data = (const char*) frame.data();
        bytes_read = 0;
        while (bytes_read < size) {
            long local_bytes_read = read(client_fd, (void*) (data + bytes_read), size - bytes_read);
            if (local_bytes_read < 0) {
                std::cerr << "Failed to read frame\n";
                break;
            }
            bytes_read += local_bytes_read;
        }


        // std::cout << "Got frame: ";
        // for (long i = 0; i < size; i++) {
        //     std::cout << (char) frame[i];
        // }
        // std::cout << "\n";

    }


    return 0;
}