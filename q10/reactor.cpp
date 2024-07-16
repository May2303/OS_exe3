#include "reactor.hpp"
#include <iostream>
#include <unistd.h>

// Constructor implementation for Reactor class
Reactor::Reactor() : max_fd(-1), running(false) {
    FD_ZERO(&read_fds); // Initialize read_fds to empty set
}

// Starts the reactor
void* Reactor::start() {
    running = true; // Set running flag to true
    return this; // Return the pointer to the current object
}

// Adds a fd to the reactor with associated callback function (func)
int Reactor::addFd(int fd, reactorFunc func) {
    std::lock_guard<std::mutex> lock(mtx); // Lock mutex to protect shared data
    if (fd < 0){
     return -1; // Invalid file descriptor
    }
    FD_SET(fd, &read_fds); // Add fd to read_fds set
    fd_map[fd] = func; // Store fd and associated function in map
    if (fd > max_fd){
    max_fd = fd; // Update max_fd if necessary
    } 
    return 0; // Success
}

// Removes fd from the reactor
int Reactor::removeFd(int fd) {
    std::lock_guard<std::mutex> lock(mtx); // Lock mutex to protect shared data
    if (fd_map.find(fd) == fd_map.end()){
         return -1; // fd not found in fd_map
    }
    FD_CLR(fd, &read_fds); // Remove fd from read_fds set
    fd_map.erase(fd); // Erase fd from fd_map

    // Update max_fd if the removed fd was the highest
    if (fd == max_fd) {
        max_fd = -1;
        for (const auto& pair : fd_map) {
            if (pair.first > max_fd) max_fd = pair.first;
        }
    }
    return 0; // Success
}

// Stops the reactor
int Reactor::stop() {
    std::lock_guard<std::mutex> lock(mtx); // Lock mutex to protect shared data
    running = false; // Set running flag to false
    return 0; // Success
}

// Runs the reactor
void Reactor::run() {
    while (running) { // Continuously loop while running is true
        fd_set temp_fds;
        {
            std::lock_guard<std::mutex> lock(mtx); // Lock mutex to protect shared data
            temp_fds = read_fds; // Copy read_fds to temp_fds for use in select
        }
        // Wait for activity on fd's - by using select
        int activity = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);

        if (activity < 0) { // Error in select
            perror("select error");
            break; // Exit loop on error
        }

        // Iterate over fd_map to find which fd triggered the activity
        for (const auto& pair : fd_map) {
            int fd = pair.first;
            if (FD_ISSET(fd, &temp_fds)) {
                pair.second(fd); // Call the associated reactor function
            }
        }
    }
}

// Function to start the reactor and return a void pointer
void* startReactor() {
    Reactor* reactor = new Reactor(); // Create a new Reactor object
    return reactor->start(); // Start the reactor and return its pointer
}

// Function to add a file descriptor to the reactor
int addFdToReactor(void* reactor, int fd, reactorFunc func) {
    Reactor* r = static_cast<Reactor*>(reactor); // Cast void pointer to Reactor pointer
    return r->addFd(fd, func); // Add fd to reactor using its member function
}

// Function to remove a fd from the reactor
int removeFdFromReactor(void* reactor, int fd) {
    Reactor* r = static_cast<Reactor*>(reactor); // Cast void pointer to Reactor pointer
    return r->removeFd(fd); // Remove fd from reactor using its member function
}

// Function to stop the reactor
int stopReactor(void* reactor) {
    Reactor* r = static_cast<Reactor*>(reactor); // Cast void pointer to Reactor pointer
    return r->stop(); // Stop the reactor using its member function
}
