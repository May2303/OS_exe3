#include "reactor.hpp"
#include <iostream>
#include <unistd.h>

// Constructor implementation
Reactor::Reactor() : max_fd(-1), running(false) {
    FD_ZERO(&read_fds);
}

// Starts the reactor
void* Reactor::start() {
    running = true;
    return this;
}

// Adds fd to the reactor
int Reactor::addFd(int fd, reactorFunc func) {
    std::lock_guard<std::mutex> lock(mtx);
    if (fd < 0) return -1;
    FD_SET(fd, &read_fds);
    fd_map[fd] = func;
    if (fd > max_fd) max_fd = fd;
    return 0;
}

// Removes fd from the reactor
int Reactor::removeFd(int fd) {
    std::lock_guard<std::mutex> lock(mtx);
    if (fd_map.find(fd) == fd_map.end()) return -1;
    FD_CLR(fd, &read_fds);
    fd_map.erase(fd);
    if (fd == max_fd) {
        max_fd = -1;
        for (const auto& pair : fd_map) {
            if (pair.first > max_fd) max_fd = pair.first;
        }
    }
    return 0;
}

// Stops the reactor
int Reactor::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    running = false;
    return 0;
}

// Runs the reactor
void Reactor::run() {
    while (running) {
        fd_set temp_fds;
        {
            std::lock_guard<std::mutex> lock(mtx);
            temp_fds = read_fds;
        }
        int activity = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);

        if (activity < 0) {
            perror("select error");
            break;
        }

        for (const auto& pair : fd_map) {
            int fd = pair.first;
            if (FD_ISSET(fd, &temp_fds)) {
                pair.second(fd); // Call the reactor function
            }
        }
    }
}

// Function implementations
void* startReactor() {
    Reactor* reactor = new Reactor();
    return reactor->start();
}

int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    Reactor* r = static_cast<Reactor*>(reactor);
    return r->addFd(fd, func);
}

int removeFdFromReactor(void* reactor, int fd) {
    Reactor* r = static_cast<Reactor*>(reactor);
    return r->removeFd(fd);
}

int stopReactor(void* reactor) {
    Reactor* r = static_cast<Reactor*>(reactor);
    return r->stop();
}
