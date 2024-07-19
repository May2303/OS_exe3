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
    Reactor* reactor = new Reactor();            // Create a new Reactor object
    reactor->running = true;                     // Set the reactor to be running
    std::thread reactorThread(&Reactor::reactorLoop, reactor);  // Create a new thread for reactor loop
    reactorThread.detach();                      // Detach thread to allow it to run independently
    return reactor;                             // Return the reactor object
}

// Add FD to Reactor
int Reactor::addFdToReactor(int fd, reactorFunc func) {
    std::lock_guard<std::mutex> lock(mtx);       // Lock mutex to protect shared data
    fdMap[fd] = func;                            // Add function callback for given file descriptor
    pollfds.push_back({fd, POLLIN, 0});          // Add file descriptor to the pollfds list for polling
    return 0;                                    
}

// Remove FD from Reactor
int Reactor::removeFdFromReactor(int fd) {
    std::lock_guard<std::mutex> lock(mtx);       // Lock mutex to protect shared data
    fdMap.erase(fd);                             // Remove function callback for given file descriptor
    pollfds.erase(std::remove_if(pollfds.begin(), pollfds.end(),
                                 [fd](const struct pollfd& pfd) { return pfd.fd == fd; }),
                  pollfds.end());                // Remove file descriptor from pollfds list
    return 0;                                    
}

// Stop Reactor
int Reactor::stopReactor() {
    running = false;                             // Set running flag to false to stop reactor loop
    return 0;                                   
}

// Reactor Loop
void Reactor::reactorLoop() {
    while (running) {                            // Loop while reactor is running
        std::vector<struct pollfd> pollfdsCopy;   // Create a copy of pollfds for thread safety
        {
            std::lock_guard<std::mutex> lock(mtx);  // Lock mutex to protect shared data
            pollfdsCopy = pollfds;                // Copy pollfds to local variable
        }
        int ret = poll(pollfdsCopy.data(), pollfdsCopy.size(), 1000);  // Poll for events with a timeout of 1000 ms
        if (ret < 0) {
            perror("poll");                      // Print error if poll fails
            break;                               // Exit loop on poll error
        }
        if (ret == 0) continue;                  // Continue loop if no events occurred
        for (const auto& pfd : pollfdsCopy) {     // Iterate through polled file descriptors
            if (pfd.revents & POLLIN) {           // Check if input event occurred
                reactorFunc func;                 // Declare function callback variable
                {
                    std::lock_guard<std::mutex> lock(mtx);  // Lock mutex to protect shared data
                    func = fdMap[pfd.fd];          // Retrieve function callback for file descriptor
                }
                if (func) func(pfd.fd);           // Call function callback if not null
            }
        }
    }
}

// Start Proactor
pthread_t Reactor::startProactor(int sockfd, proactorFunc threadFunc) {
    pthread_t tid;                               // Declare pthread variable
    if (pthread_create(&tid, nullptr, (void* (*)(void*))threadFunc, (void*)(intptr_t)sockfd) != 0) {
        perror("pthread_create");                // Print error if pthread creation fails
        exit(EXIT_FAILURE);                      
    }
    return tid;                                  // Return pthread ID
}

// Stop Proactor
int Reactor::stopProactor(pthread_t tid) {
    if (pthread_cancel(tid) != 0) {              // Cancel pthread and check for errors
        perror("pthread_cancel");                // Print error if pthread cancellation fails
        return -1;                               // Return error
    }
    if (pthread_join(tid, nullptr) != 0) {       // Join pthread and check for errors
        perror("pthread_join");                  // Print error if pthread join fails
        return -1;                              
    }
    return 0;                                    
}
