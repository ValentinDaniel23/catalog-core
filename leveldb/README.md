# LevelDB on Unikraft

Build and run LevelDB on Unikraft.
Follow the instructions below to set up, configure, build and run LevelDB.
Make sure you installed the [requirements](../README.md#requirements).

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
qemu-system-x86_64 \
    -enable-kvm \
    -nographic \
    -m 1024 \
    -cpu max \
    -kernel workdir/build/leveldb_qemu-x86_64
```

This will configure, build and run LevelDB on Unikraft.

Information about every step and about other types of builds is detailed below.

## Set Up

Set up the required repositories.
For this, you have two options:

1. Use the `setup.sh` script:

   ```console
   ./setup.sh
   ```

   It will create symbolic links to the required repositories in `../repos/`.
   Be sure to run the [top-level `setup.sh` script](../setup.sh).

   If you want use a custom variant of repositories (e.g. apply your own patch, make modifications), update it accordingly in the `../repos/` directory.

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

In the console menu interface, choose the target architecture (x86_64 or ARMv8 or ARMv7) and platform (Xen or KVM/QEMU or KVM/Firecracker).

The end result will be the creation of the `.config` configuration file.

## Build

Build the application for the current configuration:

```console
make -j $(nproc)
```

This results in the creation of the `workdir/build/` directory storing the build artifacts.
The unikernel application image file is `workdir/build/leveldb_<plat>-<arch>`, where `<plat>` is the platform name (`qemu`, `fc`, `xen`), and `<arch>` is the architecture (`x86_64` or `arm64`).

## Run

Run the resulting image using the corresponding platform tool.
Firecracker requires KVM support.
Xen requires a system with Xen installed.

A successful run prints something like:

```text
leveldb: hello=unikraft
```

This means that LevelDB opened the database, wrote a key/value pair, read it
back, and exited successfully.

You can override the database path and key/value via arguments:
`-- <db_path> <key> <value>`.

### Run on QEMU/x86_64

Run the Unikraft image:

```console
qemu-system-x86_64 \
    -enable-kvm \
    -nographic \
    -m 1024 \
    -cpu max \
    -kernel workdir/build/leveldb_qemu-x86_64
```

### Run on QEMU/ARM64

Run the Unikraft image:

```console
qemu-system-aarch64 \
    -nographic \
    -machine virt \
    -m 1024 \
    -cpu max \
    -kernel workdir/build/leveldb_qemu-arm64
```
