#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <poll.h>
#include <pthread.h>

typedef std::function<void(int)> reactorFunc;        // Define a type for reactor callback functions
typedef void* (*proactorFunc)(int sockfd);           // Define a type for proactor callback functions

class Reactor {
public:
    static Reactor* startReactor();                  // Static method to start the reactor
    int addFdToReactor(int fd, reactorFunc func);    // Method to add a file descriptor to the reactor
    int removeFdFromReactor(int fd);                 // Method to remove a file descriptor from the reactor
    int stopReactor();                               // Method to stop the reactor
    pthread_t startProactor(int sockfd, proactorFunc threadFunc);  // Method to start a proactor
    int stopProactor(pthread_t tid);                 // Method to stop a proactor
    void reactorLoop();                              // Method representing the reactor's main loop

private:
    Reactor();                                       // Private constructor
    std::map<int, reactorFunc> fdMap;                // Map to store file descriptors and their callbacks
    std::vector<struct pollfd> pollfds;              // Vector of poll file descriptors
    std::mutex mtx;                                  // Mutex to protect shared data
    std::atomic<bool> running;                       // Atomic flag to indicate if reactor is running
};

#endif // REACTOR_HPP
