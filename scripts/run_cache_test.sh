#!/usr/bin/env bash
# ============================================================
# run_cache_test.sh — start server, run cache_test.sql, show logs
#
# Usage:
#   ./scripts/run_cache_test.sh            (default port 9300)
#   ./scripts/run_cache_test.sh 9400       (custom port)
#   ./scripts/run_cache_test.sh 9300 --epoll
#   ./scripts/run_cache_test.sh 9300 --io-uring
# ============================================================

set -euo pipefail

PORT="${1:-9300}"
MODE="${2:-}"
SERVER_LOG="/tmp/flexql-cache-test-server.log"

cleanup() {
    if [ -n "${SERVER_PID:-}" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}
trap cleanup EXIT

# ---- start server ----
echo "Starting FlexQL server on port $PORT $MODE ..."
./bin/flexql-server "$PORT" $MODE >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: server failed to start. Logs:"
    cat "$SERVER_LOG"
    exit 1
fi
echo "Server PID=$SERVER_PID  log -> $SERVER_LOG"
echo ""

# ---- run SQL ----
echo "=========================================="
echo " Running scripts/cache_test.sql"
echo "=========================================="
./bin/flexql-client 127.0.0.1 "$PORT" <<'EOF'
SOURCE scripts/cache_test.sql
.exit
EOF

echo ""
echo "=========================================="
echo " Cache events from server log"
echo "=========================================="
grep "CACHE" "$SERVER_LOG" || echo "(no CACHE log lines found)"

echo ""
echo "=========================================="
echo " Summary"
echo "=========================================="
HITS=$(grep -c "CACHE HIT"       "$SERVER_LOG" 2>/dev/null || true)
MISSES=$(grep -c "CACHE MISS"    "$SERVER_LOG" 2>/dev/null || true)
INVALS=$(grep -c "CACHE INVALID" "$SERVER_LOG" 2>/dev/null || true)
echo "  HIT        : $HITS"
echo "  MISS       : $MISSES"
echo "  INVALIDATE : $INVALS"
echo ""
echo "Full server log: $SERVER_LOG"
