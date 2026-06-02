# RocksDB on Unikraft

RocksDB catalog application and validation harness for Unikraft.

## Layout

- app: `catalog-core/rocksdb`
- RocksDB port (preferred): `libs/lib-rocksdb`
- RocksDB port (legacy fallback): `libs/rocksdb`

The app `Makefile` auto-detects the library location so branch changes do not
break builds.

## Build (QEMU x86_64)

```console
./setup.sh
make distclean
UK_DEFCONFIG="$PWD/qemu.x86_64.defconfig" make defconfig
make -j "$(nproc)"
```

## Runtime CLI

```text
--db=<path>
--checkpoint=<path>
--feature=basic|batch|flush|snapshot|iterator|delete|reopen|checkpoint
--storage=<name>
--no-clean
--no-phase-logs
--help
```

Notes:

- With no `--feature` flags, the app runs the full feature-debug bundle.
- `--checkpoint=<path>` chooses where RocksDB writes the checkpoint copy.
- A standalone `--` token is accepted for compatibility, but not required.

## Feature Debugging

- `basic`: open + put/get sanity.
- `batch`: `WriteBatch` semantics.
- `flush`: explicit `Flush()` validation.
- `snapshot`: point-in-time read validation.
- `iterator`: iterator traversal validation.
- `delete`: delete semantics.
- `reopen`: reopen the DB and verify persisted state.
- `checkpoint`: create and reopen an on-disk checkpoint.

## Automated Validation

Run the full matrix automatically:

```console
./validate.sh
```

If KVM is unavailable, use TCG:

```console
QEMU_ACCEL="-accel tcg" ./validate.sh
```

If you want explicit manual commands, see `commands.txt`.

Use the app modes:

- `--persist-write` to write a marker
- reboot with the same DB path/backend
- `--persist-check` to verify marker
- `--persist-clean` to remove test data
