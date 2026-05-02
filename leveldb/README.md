# LevelDB on Unikraft

Build and run a small native LevelDB smoke test on Unikraft.

This application uses the external `lib-leveldb` port repository cloned under
`../repos/libs/leveldb`.

The `lib-leveldb` repository provides the library port itself.
This `catalog-core/leveldb` directory provides a small runnable application
that runs the shared LevelDB benchmark workload on Unikraft.

## Set Up

Run the top-level catalog setup once:

```console
cd ..
./setup.sh
cd leveldb
```

Then prepare the app workdir:

```console
./setup.sh
```

The app setup script links:

- `workdir/unikraft` from `../repos/unikraft`
- C++ runtime libraries from `../repos/libs/`
- `workdir/libs/leveldb -> ../../../repos/libs/leveldb`

## Configure

Use the usual Unikraft flow:

```console
make distclean
make menuconfig
```

Choose the target architecture and platform in `menuconfig`.
The `APPLEVELDB` option defaults to enabled, so the demo app and `LIBLEVELDB`
should already be selected.

## Build

```console
make -j $(nproc)
```

This creates the unikernel image under `workdir/build/`.

## Run on QEMU/x86_64

```console
qemu-system-x86_64 \
  -nographic \
  -m 1024 \
  -cpu max \
  -kernel workdir/build/leveldb_qemu-x86_64
```

## Run on QEMU/ARM64

```console
qemu-system-aarch64 \
  -nographic \
  -machine virt \
  -m 1024 \
  -cpu max \
  -kernel workdir/build/leveldb_qemu-arm64
```

## Expected Output

The app runs the shared LevelDB benchmark workload against the logical
`/leveldb-benchmark-db` database name in LevelDB's in-memory environment and
exits with status `0`.

In the current workspace configuration, the demo progress is emitted through
Unikraft kernel log messages instead of plain `stdout`, so the output appears
with `ERR: [appleveldb]` prefixes.

Look for output similar to:

```text
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @   33> Opening /leveldb-demo
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=READY
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=OPEN_DONE|duration_us=...
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=WRITE_DONE|duration_us=...|count=1000000
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=RANDOM_READ_DONE|duration_us=...|count=1000000
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=ITERATE_DONE|duration_us=...|count=1000000
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=DELETE_DONE|duration_us=...|count=1000000
[    0.xxxxxx] ERR:  [appleveldb] <main.cpp @ ...> BENCHMARK|EVENT=BENCH_DONE|duration_us=...|count=1
```

## Benchmarking

The shared benchmark workflow lives under:

```text
../../proiect/benchmarks/leveldb/
```

Use:

```console
python3 ../../proiect/benchmarks/leveldb/run_benchmarks.py
```

to build, run, and summarize the native versus Unikraft benchmark results.
