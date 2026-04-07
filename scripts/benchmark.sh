#!/usr/bin/env bash

set -euo pipefail

sql_file="${1:-scripts/example_queries.sql}"
port="${2:-9300}"

cleanup() {
  if [ -n "${server_pid:-}" ] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid" 2>/dev/null || true
    wait "$server_pid" 2>/dev/null || true
  fi
}

trap cleanup EXIT

rm -rf data/databases
./bin/flexql-server "$port" >/tmp/flexql-benchmark-server.log 2>&1 &
server_pid=$!
sleep 1

echo "Running SOURCE benchmark against $sql_file"
/usr/bin/time -v ./bin/flexql-client 127.0.0.1 "$port" <<EOF
SOURCE $sql_file
.exit
EOF
