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
MAIN_PORT=$(grep "\bmainport\b" "$CONFIG_FILE" | awk '{ print $2 }')
WIZ_PORT=$(grep "\bwizport\b" "$CONFIG_FILE" | awk '{ print $2 }')
LINK_PORT=$(grep "\blinkport\b" "$CONFIG_FILE" | awk '{ print $2 }')

cat << EOT > Dockerfile
FROM ${OS_ARCH}/alpine

RUN apk add build-base supervisor bash busybox-extras gdb
COPY supervisord.conf /etc/supervisord.conf

WORKDIR /amnuts
EXPOSE $MAIN_PORT
EXPOSE $WIZ_PORT
EXPOSE $LINK_PORT

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisord.conf"]
EOT

cat << EOT > docker-compose.yml
version: "3"
services:
  amnuts:
    build: ./
    image: amnuts-build
    environment:
      - "MAIN_PORT=$MAIN_PORT"
      - "WIZ_PORT=$WIZ_PORT"
      - "LINK_PORT=$LINK_PORT"
    ports:
      - "${MAIN_PORT}:${MAIN_PORT}"
      - "${WIZ_PORT}:${WIZ_PORT}"
      - "${LINK_PORT}:${LINK_PORT}"
    volumes:
      - .:/amnuts
    security_opt:
      - seccomp:unconfined
    cap_add:
      - SYS_PTRACE
EOT