#!/usr/bin/env bash

set -euo pipefail

count="${1:-100000}"
output="${2:-scripts/bulk_100000.sql}"
table="${3:-bulk_users}"

mkdir -p "$(dirname "$output")"

{
  printf "CREATE TABLE %s(ID INT PRIMARY KEY, NAME VARCHAR(255), CREATED_AT DATETIME);\n" "$table"
  i=1
  while [ "$i" -le "$count" ]; do
    printf "INSERT INTO %s VALUES (%d, 'user_%d', '2026-03-24 00:00:00');\n" "$table" "$i" "$i"
    i=$((i + 1))
  done
  printf "SELECT * FROM %s WHERE ID = 1;\n" "$table"
  printf "SELECT NAME FROM %s WHERE ID = %d;\n" "$table" "$count"
} > "$output"

printf "Generated %s with %s inserts\n" "$output" "$count"
