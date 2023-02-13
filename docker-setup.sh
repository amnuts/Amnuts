#!/usr/bin/env bash

MACHINE=$(uname -m)
if [ "$MACHINE" == "x86_64" ] || [ "$MACHINE" == "i686" ]; then
    OS_ARCH="amd64"
elif [ "$MACHINE" == "aarch64" ] || [ "$MACHINE" == "arm64" ]; then
    OS_ARCH="arm64v8"
else
    OS_ARCH="amd64"
    echo "Unknown platform - falling back to amd64"
fi

CONFIG_FILE="$(pwd)/files/datafiles/config"
MAIN_PORT=$(cat $CONFIG_FILE | grep "\bmainport\b" | awk '{ print $2 }')
WIZ_PORT=$(cat $CONFIG_FILE | grep "\bwizport\b" | awk '{ print $2 }')
LINK_PORT=$(cat $CONFIG_FILE | grep "\blinkport\b" | awk '{ print $2 }')

cat << EOT > Dockerfile
FROM ${OS_ARCH}/alpine

RUN apk add build-base supervisor bash busybox-extras
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
WORKDIR /amnuts

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]
EOT

cat << EOT > docker-compose.yml
version: "3"
services:
  amnuts:
    build: ./
    image: amnuts-build
    environment:
      - MAIN_PORT=$MAIN_PORT
      - WIZ_PORT=$WIZ_PORT
      - LINK_PORT=$LINK_PORT
    ports:
      - ${MAIN_PORT}:${MAIN_PORT}
      - ${WIZ_PORT}:${WIZ_PORT}
      - ${LINK_PORT}:${LINK_PORT}
    volumes:
      - .:/amnuts
EOT