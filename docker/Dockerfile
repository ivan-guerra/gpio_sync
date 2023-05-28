FROM debian:10.8-slim

RUN apt-get update && \
    apt-get install -y \
        cmake \
        gcc-arm-linux-gnueabihf \
        g++-arm-linux-gnueabihf \
        binutils-arm-linux-gnueabihf

RUN dpkg --add-architecture armhf && \
    apt-get update && \
    apt-get install -y libgpiod-dev:armhf

WORKDIR /opt/gpio_sync/scripts
