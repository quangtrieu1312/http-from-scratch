#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <string.h>
#include <strings.h>
#include <sstream>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>
#include <cerrno>
#include <chrono>
#include <thread>

using namespace std;

typedef unsigned long long int ull;

const char* LISTEN_ADDRESS="0.0.0.0";
uint16_t LISTEN_PORT=8082;
int serverFd;
unordered_map<int, string> clientRequest;

int getTotalCPUCores() {
    cpu_set_t cpuset;
    sched_getaffinity(0, sizeof(cpuset), &cpuset);
    return CPU_COUNT(&cpuset);
}

void addFdToEpoll(int epollFd, int fd, uint32_t events) {
    struct epoll_event event;

    memset(&event, 0, sizeof(struct epoll_event));

    event.events  = events;
    event.data.fd = fd;
    int ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    if (ret < 0) {
        printf("[FATAL] %d: Cannot add file descriptor %d to epoll fd %d: %s\n", gettid(), fd, epollFd, strerror(errno));
        exit(1);
    } else {
        printf("[DEBUG] %d: Add file descriptor %d to epoll fd %d\n", gettid(), fd, epollFd);
    }
}

void removeFdFromEpoll(int epollFd, int fd) {
    if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) < 0) {
        printf("[FATAL] %d: Cannot remove file descriptor %d from epoll fd %d: %s\n", gettid(), fd, epollFd, strerror(errno));
        exit(1);
    } else {
        printf("[DEBUG] %d: Remove file descriptor %d to epoll fd %d\n", gettid(), fd, epollFd);
    }
}

string errorResponse() {
    stringstream ss;
    string responseBody = "<!DOCTYPE html>\n<html>\n\t<p>\n\tInvalid Request\n\t</p>\n</html>";
    ss << "HTTP/1.0 400 Bad Request\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: "<<responseBody.length()<<"\r\n";
    ss << "\r\n";
    ss << responseBody;
    return ss.str();
}

void sendErrorMessage(int clientFd) {
    send(clientFd, errorResponse().c_str(), errorResponse().length(), 0);
}

vector<ull> handleCollatzSequence(ull x) {
    vector<ull> ans;
    while (x > 1) {
        ans.push_back(x);
        if (x&1) {
            x=(x<<1)+x+1;
        } else {
            x=x>>1;
        }
    }
    ans.push_back(x);
    this_thread::sleep_for(std::chrono::milliseconds(100*ans.size()));
    return ans;
}

string collatzMessage(ull collatzNum) {
    stringstream ss;
    string responseBody = "<!DOCTYPE html>\n<html>\n\t<p>\n\t[";
    vector<ull> seq = handleCollatzSequence(collatzNum);
    for (int i=0; i<seq.size(); i++) {
        responseBody += to_string(seq[i]);
        if (i!=seq.size()-1) {
            responseBody += " ";
        }
    }
    responseBody += "]\n\t</p>\n</html>";
    ss << "HTTP/1.0 200 OK\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: "<<responseBody.length()<<"\r\n";
    ss << "\r\n";
    ss << responseBody;
    return ss.str();
}

void sendCollatzSequence(int clientFd, ull collatzNum) {
    string msg = collatzMessage(collatzNum);
    int len = msg.length();
    int bufSize = 4096;
    int l = 0;
    int r = min(l+bufSize-1, len);
    char buffer[bufSize];
    //this will copy msg from [l,r) = [l,r-1] to buffer
    copy(msg.begin()+l,msg.begin()+r,buffer);
    buffer[r-l]='\0';
    ssize_t totalBytes = send(clientFd, buffer, r-l+1, 0);
    while (totalBytes > 0) {
        l+=totalBytes;
        r=min(l+bufSize-1, len);
        if (l>=r) {
            break;
        }
        copy(msg.begin()+l,msg.begin()+r,buffer);
        buffer[r-l]='\0';
        totalBytes = send(clientFd, buffer, r-l+1, 0);
    }
    if (totalBytes <= 0) {
        printf("[DEBUG] Unexpected error with fd %d while send()\n", clientFd);
    }
}

void handleClient(int clientFd, string *request) {
    stringstream ss((*request).c_str());
    string line;
    int lineNum = 0;
    ull collatzNum = 0;
    while (getline(ss, line)) {
        lineNum++;
        // Remove carriage return and newline character
        int len = line.length();
        if (line[len-1] == '\n') {
            line.resize(--len);
        }
        if (line[len-1] == '\r') {
            line.resize(--len);
        }
        // Vector of string to save tokens
        vector<string> tokens;
        // stringstream class check1
        stringstream ss2(line);
        string intermediate;
        // Tokenizing w.r.t. space ' '
        while(getline(ss2, intermediate, ' ')) {
            tokens.push_back(intermediate);
        }
        if (lineNum == 1) {
            if (tokens.size() != 3 || tokens[0]!="GET" || tokens[1][0]!='/' || tokens[1].length()==1 || tokens[2]!="HTTP/1.0") {
                goto endProcess;
            }
            for (int i=1; i<tokens[1].length(); i++) {
                if (tokens[1][i] < '0' || '9' < tokens[1][i]) {
                    goto endProcess;
                } else {
                    collatzNum *= 10;
                    collatzNum += tokens[1][i] - '0';
                }
            }
        } else if (tokens.size() == 2 && tokens[0]=="Host:" && tokens[1]!="127.0.0.1:8082") {
            goto endProcess;
        }
    }
    printf("[INFO] %d: Client request has correct format\n", gettid());
    sendCollatzSequence(clientFd, collatzNum);
    return;
endProcess:
    printf("[INFO] %d: Line #%d has invalid format: %s\n", gettid(), lineNum, line.c_str());
    sendErrorMessage(clientFd);
    return;
}

void *serverLoop(void *targs) {
    int epollFd = epoll_create(1024);
    if (epollFd < 0) {
        printf("[FATAL] %d: Something is wrong with epoll_create, return fd = %d. Killing server.\n", gettid(), epollFd);
        exit(1);
    }
    printf("[DEBUG] %d: Return epollFd = %d\n", gettid(), epollFd);
    addFdToEpoll(epollFd, serverFd, EPOLLIN);

    int timeout = 1000; /* in milliseconds */
    int maxTimeout = 30000;
    int maxEvents = 8;
    struct epoll_event events[maxEvents];

    while (true) {
        int totalEvents = epoll_wait(epollFd, events, maxEvents, timeout);
        if (totalEvents == 0) {
            timeout=min(timeout*2, maxTimeout);
            continue;
        } else if (totalEvents < 0) {
            printf("[FATAL] %d: Something is wrong with epoll. Killing server.\n", gettid());
            exit(1);
        }
        timeout = 1000;
        for (int i=0; i<totalEvents; i++) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                if (fd == serverFd) {
                    int clientFd = accept(serverFd, nullptr, nullptr);
                    clientRequest[clientFd] = "";
                    addFdToEpoll(epollFd, clientFd, EPOLLIN);
                } else {
                    // New client data available
                    ssize_t totalBytes;
                    size_t buffSize = 4096;
                    char buffer[buffSize];
                    // We only read once, if there are some unread data left in kernel buffer
                    // the next epoll_wait() will send us the same event
                    // Note:
                    // This is true since we are using default LEVER trigger EPOLL
                    // EDGE trigger EPOLL is different
                    totalBytes = recv(fd, buffer, buffSize-1, 0);
                    if (totalBytes == 0) {
                        clientRequest[fd] = "";
                        removeFdFromEpoll(epollFd, fd);
                        close(fd);
                    } else {
                        buffer[totalBytes]='\0';
                        clientRequest[fd] += string(buffer);
                        int reqLen = clientRequest[fd].length();
                        if (reqLen >= 4 && clientRequest[fd][reqLen-4] == '\r' && clientRequest[fd][reqLen-3] == '\n' && clientRequest[fd][reqLen-2] == '\r' && clientRequest[fd][reqLen-1] == '\n') {
                            handleClient(fd, &clientRequest[fd]);
                            clientRequest[fd] = "";
                            removeFdFromEpoll(epollFd, fd);
                            close(fd);
                        }
                    }
                }
            }
        }
    }
}


int main() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    if (setsockopt(serverFd,SOL_SOCKET, SO_REUSEADDR,
                   &enable, sizeof(int)) < 0) {
        printf("[FATAL] %d: Cannot set socket option with SO_REUSEADDR\n", gettid());
        exit(1);
    }
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(LISTEN_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(LISTEN_ADDRESS);
    bind(serverFd, (struct sockaddr*) &serverAddress,sizeof(serverAddress));
    listen(serverFd, 4096);
    int totalCPUCores = getTotalCPUCores();
    pthread_t threads[totalCPUCores];
    for (int i=0; i<totalCPUCores; i++) {
        pthread_create(&threads[i], NULL, &serverLoop, NULL);
    }
    for (;;) {
        pause();
    }

    return 0;
}
