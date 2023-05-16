FROM debian:10.8-slim

RUN apt-get update && \
    apt-get install -y \
        cmake \
        gcc-arm-linux-gnueabihf \
        g++-arm-linux-gnueabihf \
        binutils-arm-linux-gnueabihf

WORKDIR /opt/gpio_sync/scripts
