#include "reactor.hpp"
#include <poll.h>
#include <thread>
#include <unistd.h>
#include <algorithm>
#include <iostream>

// Constructor
Reactor::Reactor() : running(false) {}

// Start Reactor
Reactor* Reactor::startReactor() {
    Reactor* reactor = new Reactor();
    reactor->running = true;
    std::thread reactorThread(&Reactor::reactorLoop, reactor);
    reactorThread.detach();
    return reactor;
}

// Add FD to Reactor
int Reactor::addFdToReactor(int fd, reactorFunc func) {
    std::lock_guard<std::mutex> lock(mtx);
    fdMap[fd] = func;
    pollfds.push_back({fd, POLLIN, 0});
    return 0;
}

// Remove FD from Reactor
int Reactor::removeFdFromReactor(int fd) {
    std::lock_guard<std::mutex> lock(mtx);
    fdMap.erase(fd);
    pollfds.erase(std::remove_if(pollfds.begin(), pollfds.end(),
                                 [fd](const struct pollfd& pfd) { return pfd.fd == fd; }),
                  pollfds.end());
    return 0;
}

// Stop Reactor
int Reactor::stopReactor() {
    running = false;
    return 0;
}

// Reactor Loop
void Reactor::reactorLoop() {
    while (running) {
        std::vector<struct pollfd> pollfdsCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            pollfdsCopy = pollfds;
        }
        int ret = poll(pollfdsCopy.data(), pollfdsCopy.size(), 1000);
        if (ret < 0) {
            perror("poll");
            break;
        }
        if (ret == 0) continue;
        for (const auto& pfd : pollfdsCopy) {
            if (pfd.revents & POLLIN) {
                reactorFunc func;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    func = fdMap[pfd.fd];
                }
                if (func) func(pfd.fd);
            }
        }
    }
}

// Start Proactor
pthread_t Reactor::startProactor(int sockfd, proactorFunc threadFunc) {
    pthread_t tid;
    if (pthread_create(&tid, nullptr, (void* (*)(void*))threadFunc, (void*)(intptr_t)sockfd) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    return tid;
}

// Stop Proactor
int Reactor::stopProactor(pthread_t tid) {
    if (pthread_cancel(tid) != 0) {
        perror("pthread_cancel");
        return -1;
    }
    if (pthread_join(tid, nullptr) != 0) {
        perror("pthread_join");
        return -1;
    }
    return 0;
}
