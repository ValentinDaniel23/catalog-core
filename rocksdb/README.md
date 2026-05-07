# RocksDB on Unikraft

Build and run RocksDB on Unikraft.
This application uses the local `libs/lib-rocksdb` port and the upstream
RocksDB source tree at `repos/rocksdb`.

Make sure you installed the [requirements](../README.md#requirements).

## Quick Setup (aka TLDR)

Before everything, make sure you have the required repositories in `../repos/`.
If you are using this thesis workspace layout, that means having:

- `repos/unikraft`
- `repos/libs/` ports required by your chosen platform/toolchain
- `repos/rocksdb` (already present here)

To build and run the application for `x86_64` with QEMU, use the commands below:

```console
./setup.sh
make distclean
UK_DEFCONFIG="$PWD/qemu.x86_64.defconfig" make defconfig
make -j $(nproc)
./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
qemu-system-x86_64 \
    -enable-kvm \
    -nographic \
    -m 256M \
    -kernel workdir/build/rocksdb_qemu-x86_64 \
    -append "rocksdb_qemu-x86_64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] -- /data/rocksdb hello unikraft" \
    -initrd ./initrd.cpio
```

A successful run prints something like:

```text
rocksdb: hello=unikraft
```

## Notes

- The default database path is `/data/rocksdb`. The app creates `/data` if it
  does not exist.
- You can override the database path and the key/value via arguments:
  `-- <db_path> <key> <value>`.
