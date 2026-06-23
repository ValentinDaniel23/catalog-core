# Binary-Compatible LevelDB Hello on Unikraft

This application runs a small LevelDB program, compiled as an ordinary Linux
executable, on top of Unikraft using its
[binary-compatibility layer](https://unikraft.org/docs/concepts/compatibility).
Instead of porting LevelDB to Unikraft, the unmodified Linux binary is loaded and
run by the `elfloader-basic` application.

Supported on QEMU / x86_64.

## Requirements

Install the LevelDB libraries used to build the program:

```console
sudo apt install libleveldb-dev libsnappy-dev
```

Then, from the parent directory, run the top-level `setup.sh` once so the
required repositories are available.

## Build and run

```console
./setup.sh
./.scripts/build/qemu.x86_64
./.scripts/run/qemu.x86_64
```

- `setup.sh` links in the repositories this application needs.
- `.scripts/build/qemu.x86_64` builds the Linux program, packs it into the root
  filesystem, and builds the elfloader kernel.
- `.scripts/run/qemu.x86_64` boots it.

## The program

The source is in `rootfs/hello.cpp`. It opens a LevelDB database, writes a value,
reads it back, and exits with code `0` on success. To rebuild only the program:

```console
make -C rootfs/ clean all
```

## A note on output

The binary-compatibility layer does not forward the program's console output to
the screen, so you will not see any printed text when running it. The program
still executes normally; a successful run simply finishes and shuts the machine
down.
