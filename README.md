# About
This repo implements simple HTTP 1.0 client and server with Linux TCP socket

The server will try to dispatch requests and only processes HTTP 1.0 GET method for `/[number]` url. The response is a HTML text with collatz sequence starting from `[number]`

The client sends requests and benchmark average delay

# How to run
Prerequisites:
- Linux or Windows WSL2
- Docker

Single command line to build and run both client & server:
`sudo docker compose up --build -d`

To bring everything down:
`sudo docker compose down`

To monitor services' status:
`sudo docker compose ps -a` or `sudo docker compose ps a`

## SERVER
To read logs:
`sudo docker compose logs -f myserver`

To build separately with g++:
`g++ -o server main.cpp`

Notes:
- The service/container name is `myserver`
- The server can handle multiple client connections in parallel

## CLIENT
To read logs:
`sudo docker compose logs -f myclient`

To build separately with g++:
`g++ -o client main.cpp`

Notes:
- The service/container name is `myclient`
- The client cannot handle multiple connections in parallel

## LOAD TEST

- It will hog all of your CPU resources
