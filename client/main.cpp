#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <string>
#include <sstream>
#include <strings.h>
#include <chrono>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <iomanip>

#define COLLATZ_SIZE 9
using namespace std;
using TimePoint = std::chrono::system_clock::time_point;

typedef unsigned long long ull;

uint16_t serverPort = 8082;
string serverAddress = "127.0.0.1";
ull collatz[COLLATZ_SIZE] = {3148, 3744, 1237, 112, 56, 12328, 99, 104, 9999};

string collatzRequest(ull x) {
    stringstream ss;
    ss << "GET /" << x <<" HTTP/1.0\r\n";
    ss << "Host: 127.0.0.1:8082\r\n";
    ss << "\r\n";
    return ss.str();
}

void sendRequest(int fd, string msg) {
    int len=msg.length();
    int bufSize = 4096;
    char buffer[bufSize];
    int l=0;
    int r=min(l+bufSize-1, len);
    //this will copy msg from [l,r) = [l,r-1] to buffer
    copy(msg.begin()+l,msg.begin()+r,buffer);
    buffer[r-l]='\0';
    ssize_t totalBytes = send(fd, buffer, r-l+1, 0);
    while (totalBytes > 0) {
        l+=totalBytes;
        r=min(l+bufSize-1, len);
        if (l>r) {
            break;
        }
        //this will copy msg from [l,r) = [l,r-1] to buffer
        copy(msg.begin()+l,msg.begin()+r,buffer);
        buffer[r-l]='\0';
        totalBytes = send(fd, buffer, r-l+1, 0);
    }
    if (totalBytes <= 0) {
        //printf("[DEBUG] Unexpected error with fd %d while send()\n", fd);
    }
}

string readResponse(int fd) {
    string resp;
    int bufSize = 4096;
    char buffer[bufSize];
    ssize_t totalBytes = 0;
    totalBytes = recv(fd, buffer, 4096, 0); 
    while (totalBytes > 0) {
        resp = resp + string(buffer);
        totalBytes = recv(fd, buffer, sizeof(buffer), 0); 
    }
    if (totalBytes < 0) {
        //printf("[DEBUG] Unexpected error with fd %d while recv()\n", fd);
    } else {
        //printf("[DEBUG] End of response\n");
    }
    string ret = "";
    bool isval=false;
    for (int i=0; i<resp.length(); i++) {
        if (resp[i] == '[') {
            isval = true;
        }
        if (isval) {
            ret += resp[i];
        }
        if (resp[i] == ']') {
            isval = false;
        }
    }
    return ret;
}

bool isIpv4(string x) {
    struct sockaddr_in sa;
    int ret = inet_pton(AF_INET, x.c_str(), &(sa.sin_addr));
    return ret != 0;
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverAddress.c_str());

    string msg, resp;
    ull sumdiff=0;
    for (int i=0; i<COLLATZ_SIZE; i++) {
        stringstream ss;
        ss << "Number: " << collatz[i] << ", ";
        msg = collatzRequest(collatz[i]);
        // TCP 3 way handshake
        //printf("[INFO] Preparing new connection for %d-th request, collatz number = %lld\n", i+1, collatz[i]);
        connect(clientSocket, (struct sockaddr*)&serverAddr,sizeof(serverAddr));
        printf("######################\n%s", msg.c_str());
        sendRequest(clientSocket, msg);
        char sbuffer[26];
        int smillisec;
        struct tm* stm_info;
        struct timeval stv;

        gettimeofday(&stv, NULL);

        smillisec = lrint(stv.tv_usec/1000.0); // Round to nearest millisec
        if (smillisec>=1000) { // Allow for rounding up to nearest second
            smillisec -=1000;
            stv.tv_sec++;
        }

        stm_info = localtime(&stv.tv_sec);

        strftime(sbuffer, 26, "%Y:%m:%d %H:%M:%S", stm_info);
        ss << "Sent: " << sbuffer << "." << setfill('0') << setw(3) << smillisec << ", ";
        resp = readResponse(clientSocket);
        char ebuffer[26];
        int emillisec;
        struct tm* etm_info;
        struct timeval etv;
        gettimeofday(&etv, NULL);

        emillisec = lrint(etv.tv_usec/1000.0); // Round to nearest millisec
        if (emillisec>=1000) { // Allow for rounding up to nearest second
            emillisec -=1000;
            etv.tv_sec++;
        }

        etm_info = localtime(&etv.tv_sec);

        strftime(ebuffer, 26, "%Y:%m:%d %H:%M:%S", etm_info);
        ss << "Received: " << ebuffer << "." << setfill('0') << setw(3) << emillisec << ", ";
        auto diff = (etv.tv_sec - stv.tv_sec)*1000 + (emillisec - smillisec);
        sumdiff += diff;
        ss << "Delay: " << diff/1000 << "." << setfill('0') << setw(3) << diff%1000 << " seconds";
        close(clientSocket);
        if (i != sizeof(collatz) - 1) {
            clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        }
        printf("%s\n", resp.c_str());
        printf("%s\n", ss.str().c_str());
    }
    ull average = floor(1.0L*sumdiff/COLLATZ_SIZE);
    printf("+++++++++++++++++++++++++++\n");
    printf("Average delay is %lld.%03lld seconds\n", average/1000, average%1000);
    printf("+++++++++++++++++++++++++++\n");
    return 0;
}
