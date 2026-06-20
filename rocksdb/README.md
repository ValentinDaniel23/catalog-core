# RocksDB on Unikraft

Build and run RocksDB on Unikraft.
Follow the instructions below to set up, configure, build and run RocksDB.
Make sure you installed the [requirements](../README.md#requirements).

This is a C++ library port. By default the application links the RocksDB
library and prints a short message. An optional validation suite (the upstream
RocksDB examples) can be enabled from the configuration; see the
["Test" section](#test).

## Quick Setup (aka TLDR)

For a quick setup, run the commands below.
Note that you still need to install the [requirements](../README.md#requirements).
Before everything, make sure you run the [top-level `setup.sh` script](../setup.sh).

To build and run the application for `x86_64`, use the commands below:

```console
./setup.sh
make distclean
UK_DEFCONFIG="$PWD/qemu.x86_64.defconfig" make defconfig
make -j $(nproc)
test -f initrd.cpio || ./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
qemu-system-x86_64 \
    -nographic \
    -kernel workdir/build/rocksdb_qemu-x86_64 \
    -append "rocksdb_qemu-x86_64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] --" \
    -initrd ./initrd.cpio
```

A successful run prints:

```text
hello from the RocksDB Unikraft app
```

To do the same for `AArch64`, run the commands below:

```console
./setup.sh
make distclean
UK_DEFCONFIG="$PWD/qemu.arm64.defconfig" make defconfig
make -j $(nproc)
test -f initrd.cpio || ./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
qemu-system-aarch64 \
    -nographic \
    -machine virt \
    -cpu max \
    -kernel workdir/build/rocksdb_qemu-arm64 \
    -append "rocksdb_qemu-arm64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] --" \
    -initrd ./initrd.cpio
```

Information about every step is detailed below.

## Set Up

Set up the required repositories.
For this, you have two options:

1. Use the `setup.sh` script:

   ```console
   ./setup.sh
   ```

   It will create symbolic links to the required repositories in `../repos/`.
   Be sure to run the [top-level `setup.sh` script](../setup.sh).

   If you want, you can use a custom variant of repositories (e.g. apply your own
   patch, make modifications), update it accordingly in the `../repos/` directory.

1. Have your custom setup of repositories in the `workdir/` directory.
   Clone, update and customize repositories to your own needs.

## Clean

While not strictly required, it is safest to clean the previous build artifacts:

```console
make distclean
```

## Configure

To configure the kernel, use:

```console
make menuconfig
```

In the console menu interface, choose the target architecture (`x86_64` or
`ARMv8`) and platform (`KVM/QEMU` or `KVM/Firecracker`).

The end result will be the creation of the `.config` configuration file.

This app tree ships the local presets below:
`qemu.x86_64.defconfig`, `qemu.arm64.defconfig`, `fc.x86_64.defconfig`,
`fc.arm64.defconfig`. Load one with:

```console
UK_DEFCONFIG="$PWD/qemu.x86_64.defconfig" make defconfig
```

## Build

Build the application for the current configuration:

```console
make -j $(nproc)
```

This results in the creation of the `workdir/build/` directory storing the build
artifacts. The unikernel application image file is
`workdir/build/rocksdb_<plat>-<arch>`, where `<plat>` is the platform name
(`qemu` or `fc`), and `<arch>` is the architecture (`x86_64` or `arm64`).

### Use a Different Compiler

If you want to use a different compiler, such as Clang or a different GCC
version, pass the `CC` variable to `make`:

```console
make properclean
make CC=clang -j $(nproc)
```

Note that Clang >= 14 and GCC >= 8 are required to build Unikraft.

### Build the Filesystem

The root filesystem is packed into `initrd.cpio`, an initial RAM disk CPIO file.
Use the command below to (re)build it from the `rootfs/` directory:

```console
rm -f initrd.cpio
./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
```

At boot the initrd is extracted into a writable `ramfs` mounted at `/` via the
`vfs.fstab=[ "initrd0:/:extract::ramfs=1:" ]` argument. The validation suite
creates its databases under this root.

## Run

Run the resulting image using the corresponding platform tool. Firecracker
requires KVM support.

### Run on QEMU/x86_64

```console
qemu-system-x86_64 \
    -nographic \
    -kernel workdir/build/rocksdb_qemu-x86_64 \
    -append "rocksdb_qemu-x86_64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] --" \
    -initrd ./initrd.cpio
```

### Run on QEMU/ARM64

```console
qemu-system-aarch64 \
    -nographic \
    -machine virt \
    -cpu max \
    -kernel workdir/build/rocksdb_qemu-arm64 \
    -append "rocksdb_qemu-arm64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] --" \
    -initrd ./initrd.cpio
```

### Run on Firecracker/x86_64

```console
rm -f firecracker.socket
firecracker-x86_64 --config-file fc.x86_64.json --api-sock firecracker.socket
```

The shipped `fc.x86_64.json` mounts the initrd as rootfs (via `vfs.fstab` in its
`boot_args`), so build `initrd.cpio` first (see
["Build the Filesystem"](#build-the-filesystem)). The user running the command
must be able to use KVM.

### Run on Firecracker/ARM64

```console
rm -f firecracker.socket
firecracker-aarch64 --config-file fc.arm64.json --api-sock firecracker.socket
```

As for `x86_64`, build `initrd.cpio` first (see
["Build the Filesystem"](#build-the-filesystem)); the shipped `fc.arm64.json`
mounts it as rootfs via its `boot_args`. The user running the command must be
able to use KVM.

## Test

By default only the library is linked and the app just prints a message — the
validation suite is **not** part of the binary. To run the validation suite (the
official upstream RocksDB examples, driven by the library's `rocksdb_test_main()`),
enable it from the configuration.

The suite spawns many background threads and writes to the in-memory root, so
also raise the maximum thread id and give the VM more memory. RocksDB's workload
is heavy, so enable KVM:

```console
make distclean
cp qemu.x86_64.defconfig /tmp/rocksdb-test.defconfig
printf 'CONFIG_LIBROCKSDBTEST=y\nCONFIG_LIBPOSIX_PROCESS_MAX_PID=1024\n' >> /tmp/rocksdb-test.defconfig
UK_DEFCONFIG=/tmp/rocksdb-test.defconfig make defconfig
make -j $(nproc)
test -f initrd.cpio || ./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
qemu-system-x86_64 \
    -enable-kvm -cpu max \
    -nographic \
    -m 2048 \
    -kernel workdir/build/rocksdb_qemu-x86_64 \
    -append "rocksdb_qemu-x86_64 vfs.fstab=[ \"initrd0:/:extract::ramfs=1:\" ] --" \
    -initrd ./initrd.cpio
```

A successful run shows the message followed by the suite summary:

```text
hello from the RocksDB Unikraft app
[1/8] simple_example                             ... OK
...
=== Summary: 8 passed, 0 failed out of 8 ===
main returned 0
```

To pick individual examples instead of the whole bundle, disable
`CONFIG_LIBROCKSDBTEST_ALL` and select the ones you want
(`CONFIG_LIBROCKSDBTEST_SIMPLE`, `CONFIG_LIBROCKSDBTEST_C_SIMPLE`, ...) via
`make menuconfig`. The validation suite lives in the `lib-rocksdb` library
(gated by `CONFIG_LIBROCKSDBTEST`), not in this application.

### Close QEMU

To close the QEMU virtual machine, use the `Ctrl+a x` keyboard shortcut;
that is, press the `Ctrl` and `a` keys at the same time and then, separately,
press the `x` key.

## Use a Different Filesystem Type for QEMU

The initrd root above is in-memory and ephemeral. You can instead back the root
with [`9pfs`](https://github.com/unikraft/unikraft/tree/staging/lib/9pfs) over a
host directory, which gives **persistent** storage: databases written by RocksDB
survive on the host. Note that 9pfs does not work with Firecracker.

Enable the 9p stack in the configuration (e.g. via `make menuconfig` or by
appending to the defconfig):

```text
CONFIG_LIBUK9P=y
CONFIG_LIB9PFS=y
CONFIG_LIBVIRTIO_9P=y
CONFIG_LIBVIRTIO_PCI=y
```

Then go through the [configure](#configure) and [build](#build) steps and run
with a host directory shared over 9p (no initrd needed):

```console
mkdir -p 9pfs-root
qemu-system-x86_64 \
    -enable-kvm -cpu max \
    -nographic \
    -m 2048 \
    -kernel workdir/build/rocksdb_qemu-x86_64 \
    -append "rocksdb_qemu-x86_64 vfs.fstab=[ \"fs0:/:9pfs:::\" ] --" \
    -fsdev local,id=myid,path=$(pwd)/9pfs-root/,security_model=none \
    -device virtio-9p-pci,fsdev=myid,mount_tag=fs0
```

## Clean Up

In order to remove the build artifacts, use:

```console
make clean
```

In order to remove fetched files also, that is the removal of the
`workdir/build/` directory, use:

```console
make properclean
```

In order to remove the generated `.config` file as well, use:

```console
make distclean
```
