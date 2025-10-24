from ubuntu:latest as builder
run apt-get update -y && apt upgrade -y
run apt install -y build-essential
run mkdir -p /src /build
copy main.cpp /src/main.cpp
run cd /src && g++ -o /build/client main.cpp && chmod +x /build/client

from ubuntu:latest as runner
run apt-get update -y && apt upgrade -y
copy --from=builder /build/client /client
cmd ["/client"]
