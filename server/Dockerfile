from ubuntu:latest as builder
run apt-get update -y && apt upgrade -y
run apt install -y build-essential
run mkdir -p /src /build
copy main.cpp /src/main.cpp
run cd /src && g++ -o /build/server main.cpp && chmod +x /build/server

from ubuntu:latest as runner
run apt-get update -y && apt upgrade -y
run ulimit -n 500000
copy --from=builder /build/server /server
cmd ["/server"]
