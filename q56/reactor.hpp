#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <map>
#include <sys/select.h>
#include <mutex>

// typedef for the reactor function
typedef void (*reactorFunc)(int fd);

// Reactor class definition
class Reactor {
public:
    // Constructor
    Reactor();

    // Starts the reactor
    void* start();

    // Adds fd to the reactor
    int addFd(int fd, reactorFunc func);

    // Removes fd from the reactor
    int removeFd(int fd);

    // Stops the reactor
    int stop();

    // Runs the reactor
    void run();

private:
    fd_set read_fds;
    int max_fd;
    bool running;
    std::map<int, reactorFunc> fd_map;
    std::mutex mtx; // mutex for thread safety
};

// Function prototypes
void* startReactor();
int addFdToReactor(void* reactor, int fd, reactorFunc func);
int removeFdFromReactor(void* reactor, int fd);
int stopReactor(void* reactor);

#endif // REACTOR_HPP
