#!/usr/bin/env bash

#Linux has max open file limit which also apply to socket
#We want to remove this limit just in case we need to stress test
iteration=0
maxIteration=10000
ulimit -n $((maxIteration*2))
cleanup() {
    # Kill every established connection that involves tcp:8082
    # except for the server PID that listens on tcp:8082
    echo 'Number of established connection is: '$(lsof -sTCP:ESTABLISHED -iTCP:8082 -t | wc -w)''
    kill -9 $(lsof -sTCP:ESTABLISHED -iTCP:8082 -t)
    exit
}
trap cleanup SIGINT
while [[ $iteration -lt $maxIteration ]]; do
    iteration=$((iteration + 1))
    if [[ $((iteration % 100)) -eq 0 ]]; then
        echo "Completed $((iteration*100/maxIteration))%: $iteration/$maxIteration"
        echo 'Number of established connection is: '$(lsof -sTCP:ESTABLISHED -iTCP:8082 -t | wc -w)''
    fi
    #Open connection and do
    #We want to see the server handle multiple connections at the same time
    nohup nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /3148 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /3744 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /1237 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /112 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /56 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /12328 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /99 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /104 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /13864897 HTTP/1.0\r\nHost: 127.0.0.1:802\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /7894631276 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /1 HTTP/1.0\r\nHost: 127.0.3.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /dfief45 HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET / HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /index.html HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /a HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
    printf "GET /99df HTTP/1.0\r\nHost: 127.0.0.1:8082\r\n\r\n" | nc 127.0.0.1 8082 >/dev/null 2>&1 & disown
done
sleep 5
cleanup
