CXX := g++
CXXFLAGS := -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -pthread -Iinclude -Isrc
LDFLAGS := -pthread

COMMON_SOURCES := \
	src/network/socket_server.cpp \
	src/network/socket_client.cpp \
	src/network/event_loop.cpp \
	src/parser/sql_parser.cpp \
	src/query/executor.cpp \
	src/query/vectorized_executor.cpp \
	src/query/query_plan.cpp \
	src/storage/table.cpp \
	src/storage/mmap_reader.cpp \
	src/storage/schema.cpp \
	src/storage/row.cpp \
	src/index/btree_index.cpp \
	src/index/bloom_filter.cpp \
	src/cache/lru_cache.cpp \
	src/expiration/ttl_manager.cpp \
	src/concurrency/lock_manager.cpp \
	src/concurrency/lockfree_append.cpp \
	src/concurrency/thread_pool.cpp \
	src/memory/arena_allocator.cpp \
	src/utils/logger.cpp \
	src/utils/helpers.cpp

CLIENT_SOURCES := \
	src/client/main.cpp \
	src/client/repl.cpp \
	src/client/api.cpp \
	$(COMMON_SOURCES)

BENCHMARK_REAL_SOURCES := \
	src/client/benchmark_real.cpp \
	src/client/api.cpp \
	$(COMMON_SOURCES)

SERVER_SOURCES := \
	src/server/main.cpp \
	src/server/server.cpp \
	$(COMMON_SOURCES)

all: bin/flexql-server bin/flexql-client bin/benchmark_flexql_real

bin/flexql-server: $(SERVER_SOURCES)
	@mkdir -p bin build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

bin/flexql-client: $(CLIENT_SOURCES)
	@mkdir -p bin build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

bin/benchmark_flexql_real: $(BENCHMARK_REAL_SOURCES)
	@mkdir -p bin build
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f bin/flexql-server bin/flexql-client bin/test_queries bin/benchmark_flexql_real
	rm -rf data/databases

.PHONY: all clean
