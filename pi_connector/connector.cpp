#include <uvgrtp/lib.hh>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>

void receive_frame(void* arg, uvgrtp::frame::rtp_frame* frame);
void write_full_frame(long camID);
void handle_sigpipe(int signum);

const std::string local_address = "127.0.0.1";

// each port/frame corresponds to a different camera
const int ports[] = {9091, 9092};
std::vector<unsigned char> frames[2];

// this local socket connects this process to the server process
const std::string socket_path = "/tmp/wireless_camera_socket";
int socket_fd = -1;
std::mutex socket_mutex;

// Set up UVGRTP session, creating two streams (could be increased), each with a different port. Each stream
// should correspond to a different camera. 
int main(int argc, char** argv) {

    // set up signal handler for broken pipe
    struct sigaction sigpipe_action;
    sigpipe_action.sa_handler = handle_sigpipe;
    sigpipe_action.sa_flags = 0;
    sigaction(SIGPIPE, &sigpipe_action, NULL);

    // Set up local socket (outgoing information)
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return -1;
    }

    sockaddr_un address = {0};
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1);

    // Connect to local socket. This will loop until the socket can connect (the server is running).
    std::cout << "Attempting to connect to local socket at " << socket_path << "\n";
    while (true) {
        int status = connect(socket_fd, (struct sockaddr*)&address, sizeof(address));
        if (status == 0) {
            std::cout << "Connected!\n";
            break;
        }
        sleep(0.2);
    }

    // Set up UVGRPT (incoming information)
    uvgrtp::context context;
    uvgrtp::session* session = context.create_session(local_address);
    if (session == NULL) {
        std::cerr << "Failed to create session\n";
        return -1;
    }
    
    uvgrtp::media_stream* streams[2] = {NULL};
    for (int i = 0; i < 2; i++) {
        // create a media stream for each camera (differentiated by port)
        streams[i] = session->create_stream(ports[i], RTP_FORMAT_GENERIC, RCE_RECEIVE_ONLY);
        if (streams[i] == NULL) {
            std::cerr << "Failed to create stream\n";
            return -1;
        }

        // install each receive function with the camera ID as the only argument
        rtp_error_t error = streams[i]->install_receive_hook((void*) i, receive_frame);
        if (error != RTP_OK) {
            std::cerr << "Failed to install receive hook\n";
            return -1;
        }

        frames[i].clear();
    }


    while (true) {
        ;   // work forever
    }

    return 0;
}

// handles receiving a frame in RTP. Assumes that the frame holds generic data,
// and that marker == 1 when the frame is last in a sequence.
void receive_frame(void* arg, uvgrtp::frame::rtp_frame* frame) {

    long camID = (long) arg;

    unsigned int marker = frame->header.marker;
    unsigned int timestamp = frame->header.timestamp;

    // std::cout << "Received frame from camera " << camID << "\n";
    // std::cout << "Size: " << frame->payload_len << "\n";
    // std::cout << "marker " << (int) frame->header.marker << "\n";
    // std::cout << "timestamp " << frame->header.timestamp << "\n";

    // add data to the full frame
    for (long i = 0; i < frame->payload_len; i++) {
        frames[camID].push_back(frame->payload[i]);
    }

    // if the frame is complete, send it
    if (marker == 1) {
        write_full_frame(camID);
        // std::cout << "Sent frame from camera " << camID << "\n";
    }


}

// sends the frame via unix socket to the server process.
// For now, writes the camID (long), the size of the frame (long), and the frame itself (bytes).
void write_full_frame(long camID) {
    // std::cout << "Sending frame from " << camID << "\n";
    // std::cout << "size: " << frames[camID].size() << "\n";

    if (socket_fd < 0) {
        std::cerr << "Socket not connected\n";
        return;
    }

    socket_mutex.lock();

    int bytes_sent = 0;
    // write the camera ID
    send(socket_fd, &camID, sizeof(camID), 0);

    // write the size of the frame
    long size = frames[camID].size();
    send(socket_fd, &size, sizeof(size), 0);

    // write the frame itself
    const char* data = (const char*) frames[camID].data();
    while (bytes_sent < size) {
        
        long sent = send(socket_fd, data + bytes_sent, size - bytes_sent, 0);
        if (sent < 0) {
            std::cerr << "Failed to send frame\n";
            break;
        }
        bytes_sent += sent;
    }


    frames[camID].clear();

    socket_mutex.unlock();

}

void handle_sigpipe(int signum) {
    std::cerr << "Lost connection to server process (broken pipe)\n";
    close(socket_fd);
    exit(-1);
}