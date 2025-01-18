#!/usr/bin/env bash

CONFIG_FILE="$(pwd)/files/datafiles/config"
MAIN_PORT=$(grep "\bmainport\b" "$CONFIG_FILE" | awk '{ print $2 }')
WIZ_PORT=$(grep "\bwizport\b" "$CONFIG_FILE" | awk '{ print $2 }')
LINK_PORT=$(grep "\blinkport\b" "$CONFIG_FILE" | awk '{ print $2 }')

cat << EOT > Dockerfile
FROM alpine:latest

RUN apk add --no-cache build-base bash busybox-extras clang gdb lldb supervisor
COPY supervisord.conf /etc/supervisord.conf

WORKDIR /amnuts
EXPOSE $MAIN_PORT
EXPOSE $WIZ_PORT
EXPOSE $LINK_PORT

CMD ["/usr/bin/supervisord", "-c", "/etc/supervisord.conf"]
EOT

cat << EOT > docker-compose.yml
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
