#!/usr/bin/env bash

CONFIG_FILE="$(pwd)/files/datafiles/config.yaml"
if [ ! -f "$CONFIG_FILE" ]; then
    CONFIG_FILE="$(pwd)/files/datafiles/config.yaml.sample"
fi

read_port() {
    awk -v key="$1" '
        /^[A-Za-z]/                      { in_server = ($0 ~ /^server:/); in_ports = 0 }
        in_server && /^  [A-Za-z]/       { in_ports = ($0 ~ /^  ports:[[:space:]]*$/) }
        in_ports && /^    [A-Za-z]/ {
            split($0, parts, ":")
            k = parts[1]; sub(/^[[:space:]]+/, "", k)
            v = parts[2]; sub(/^[[:space:]]+/, "", v); sub(/[[:space:]]+$/, "", v)
            if (k == key) { print v; exit }
        }
    ' "$CONFIG_FILE"
}

MAIN_PORT=$(read_port main)
WIZ_PORT=$(read_port wiz)
LINK_PORT=$(read_port link)

cat << EOT > Dockerfile
FROM alpine:latest

RUN apk add --no-cache build-base bash busybox-extras clang gdb lldb supervisor python3 py3-yaml
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
