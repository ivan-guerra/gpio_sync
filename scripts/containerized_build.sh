#!/bin/bash

source config.sh

GSYNC_IMAGE="gsync:latest"

docker run --rm -it \
    --privileged \
    -u $(id -u ${USER}):$(id -g ${USER}) \
    -v "${GSYNC_PROJECT_PATH}":/opt/gpio_sync \
    ${GSYNC_IMAGE}
