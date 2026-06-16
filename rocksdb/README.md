# RocksDB on Unikraft

Build and run RocksDB on Unikraft.
Follow the instructions below to set up, configure, build and run RocksDB.
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
	 -monitor none -serial stdio -display none \
	 -m 512 \
	 -cpu max \
	 -kernel workdir/build/rocksdb_qemu-x86_64 \
	 -append "--storage=ramfs --db=/rocksdb-demo --checkpoint=/rocksdb-demo-checkpoint --all-features"
```

This will configure, build and run the RocksDB validation harness on Unikraft.
A successful run ends with messages such as:

```text
rocksdb: feature <name> passed
rocksdb: feature debug run passed
main returned 0
```

To do the same for `AArch64`, run the commands below:

```console
./setup.sh
make distclean
UK_DEFCONFIG="$PWD/qemu.arm64.defconfig" make defconfig
make -j $(nproc)
qemu-system-aarch64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-machine virt \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-arm64 \
	-append "--storage=ramfs --db=/rocksdb-demo --checkpoint=/rocksdb-demo-checkpoint --all-features"
```

Information about every step and about the other supported local run targets is detailed below.

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

In the console menu interface, choose the target architecture (`x86_64` or `ARMv8`) and platform (`KVM/QEMU` or `KVM/Firecracker`).

The end result will be the creation of the `.config` configuration file.

Notes:

- This app tree ships the local presets below:
	`qemu.x86_64.defconfig`, `qemu.arm64.defconfig`, `fc.x86_64.defconfig`, `fc.arm64.defconfig`.
- `UK_DEFCONFIG="$PWD/<name>.defconfig" make defconfig` loads the selected preset into `.config`.

Use the shipped presets with one of the commands below:

```console
UK_DEFCONFIG="$PWD/qemu.x86_64.defconfig" make defconfig
UK_DEFCONFIG="$PWD/qemu.arm64.defconfig" make defconfig
UK_DEFCONFIG="$PWD/fc.x86_64.defconfig" make defconfig
UK_DEFCONFIG="$PWD/fc.arm64.defconfig" make defconfig
```

## Build

Build the application for the current configuration:

```console
make -j $(nproc)
```

This results in the creation of the `workdir/build/` directory storing the build artifacts.
The unikernel application image file is `workdir/build/rocksdb_<plat>-<arch>`, where `<plat>` is the platform name (`qemu` or `fc`), and `<arch>` is the architecture (`x86_64` or `arm64`).

### Use a Different Compiler

If you want to use a different compiler, such as Clang or a different GCC version, pass the `CC` variable to `make`.

To build with Clang, use the commands below:

```console
make properclean
make CC=clang -j $(nproc)
```

Note that Clang >= 14 is required to build Unikraft.

To build with another GCC version, use the commands below:

```console
make properclean
make CC=gcc-<version> -j $(nproc)
```

where `<version>` is the GCC version, such as `11`, `12`.

Note that GCC >= 8 is required to build Unikraft.

### Build the Filesystem

For `Firecracker` or any `QEMU` run that uses `--storage=initrd`, pack the root filesystem into `initrd.cpio` using:

```console
rm -f initrd.cpio
./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
```

## Runtime CLI

The RocksDB app accepts the arguments below:

```text
--db=<path>
--checkpoint=<path>
--feature=all|basic|batch|flush|snapshot|iterator|delete|reopen|checkpoint
--feature=transaction|backup|cache-dump-load|secondary-index
--feature=merge|column-family|sst-ingest|write-batch-with-index
--feature=optimistic-transaction|compaction-filter
--all-features
--storage=ramfs|9p|blk|initrd
--no-clean
--no-phase-logs
--help
```

Notes:

- With no `--feature` flags, the app runs the full feature bundle.
- Repeat `--feature=<name>` to run multiple slices in one boot.
- `--checkpoint=<path>` chooses where RocksDB writes the checkpoint copy.
- `--storage=<name>` is a label for the selected runtime backend; the actual backend is decided by the platform boot configuration.
- A standalone `--` token is accepted for compatibility, but not required.

The available feature slices are grouped as follows:

- Core: `basic`, `batch`, `flush`, `snapshot`, `iterator`, `delete`, `reopen`, `checkpoint`
- Storage utilities: `transaction`, `backup`, `cache-dump-load`, `secondary-index`
- Advanced utilities: `merge`, `column-family`, `sst-ingest`, `write-batch-with-index`, `optimistic-transaction`, `compaction-filter`

## Run

Run the resulting image using the corresponding platform tool.
Firecracker requires KVM support.

The RocksDB harness exits after the selected feature slice or bundle completes.
A successful run will show messages such as the ones below:

```text
rocksdb: feature basic passed
rocksdb: feature debug run passed
main returned 0
```

### Run on QEMU/x86_64

```console
qemu-system-x86_64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-x86_64 \
	-append "--storage=ramfs --db=/rocksdb-demo --checkpoint=/rocksdb-demo-checkpoint --all-features"
```

If KVM is unavailable, replace `-enable-kvm` with `-accel tcg`.

### Run on QEMU/ARM64

```console
qemu-system-aarch64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-machine virt \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-arm64 \
	-append "--storage=ramfs --db=/rocksdb-demo --checkpoint=/rocksdb-demo-checkpoint --all-features"
```

If KVM is unavailable, replace `-enable-kvm` with `-accel tcg`.

### Run on Firecracker/x86_64

First build the initrd:

```console
rm -f initrd.cpio
./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
```

Now run the Unikraft image:

```console
rm -f firecracker.socket
firecracker-x86_64 --config-file fc.x86_64.json --api-sock firecracker.socket
```

The shipped `fc.x86_64.json` template boots the `basic` feature slice with `--storage=initrd`.
Edit `boot_args` in `fc.x86_64.json` if you want a different feature set or DB path.
The shipped preset already mounts the provided initrd as rootfs via `.config`, so the template only passes RocksDB application arguments after `--`.

### Run on Firecracker/ARM64

First build the initrd:

```console
rm -f initrd.cpio
./workdir/unikraft/support/scripts/mkcpio initrd.cpio ./rootfs/
```

Now run the Unikraft image:

```console
rm -f firecracker.socket
firecracker-aarch64 --config-file fc.arm64.json --api-sock firecracker.socket
```

The shipped `fc.arm64.json` template boots the `basic` feature slice with `--storage=initrd`.
Edit `boot_args` in `fc.arm64.json` if you want a different feature set or DB path.
The shipped preset already mounts the provided initrd as rootfs via `.config`, so the template only passes RocksDB application arguments after `--`.

## Examples

Run the help output:

```console
qemu-system-x86_64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-x86_64 \
	-append "--help"
```

Run a single feature slice:

```console
qemu-system-x86_64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-x86_64 \
	-append "--storage=ramfs --db=/rocksdb-demo --feature=basic"
```

Run a mixed slice set explicitly:

```console
qemu-system-x86_64 \
	-enable-kvm \
	-monitor none -serial stdio -display none \
	-m 512 \
	-cpu max \
	-kernel workdir/build/rocksdb_qemu-x86_64 \
	-append "--storage=ramfs --db=/rocksdb-demo --checkpoint=/rocksdb-demo-checkpoint --feature=merge --feature=column-family --feature=sst-ingest"
```

For more explicit commands, see `commands.txt`.

## Clean Up

Doing a new configuration, or a new build may require cleaning up the configuration and build artifacts.

In order to remove the build artifacts, use:

```console
make clean
```

In order to remove fetched files also, that is the removal of the `workdir/build/` directory, use:

```console
make properclean
```

In order to remove the generated `.config` file as well, use:

```console
make distclean
```
