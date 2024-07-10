#ifndef REACTOR_HPP
#define REACTOR_HPP

#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <poll.h>
#include <pthread.h>

typedef std::function<void(int)> reactorFunc;
typedef void* (*proactorFunc)(int sockfd);

class Reactor {
public:
    static Reactor* startReactor();
    int addFdToReactor(int fd, reactorFunc func);
    int removeFdFromReactor(int fd);
    int stopReactor();
    pthread_t startProactor(int sockfd, proactorFunc threadFunc);
    int stopProactor(pthread_t tid);
    void reactorLoop();

private:
    Reactor();
    std::map<int, reactorFunc> fdMap;
    std::vector<struct pollfd> pollfds;
    std::mutex mtx;
    std::atomic<bool> running;
};

#endif // REACTOR_HPP
