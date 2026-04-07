# FlexQL

FlexQL is a flexible SQL-like database driver implemented in C++17. It follows
a client-server architecture and supports core database features such as table
creation, insertion, selection, joins, primary-key indexing, caching, and row
expiration.

## Build

```bash
make
```

## Run

You must start the server first, then run the benchmark or unit-test workflow
from another terminal.

### Terminal 1

```bash
./server
```

### Terminal 2

Run unit tests only:

```bash
./benchmark --unit-test
```

Run benchmark + unit tests with default row count:

```bash
./benchmark
```

Run benchmark + unit tests with custom row count:

```bash
./benchmark 200000
```

## Typical Workflow

```bash
make

# Terminal 1
./server

# Terminal 2
./benchmark --unit-test
./benchmark
./benchmark 200000
```
