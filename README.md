# Aegis

Aegis is a small Linux container runtime prototype written in C++.

The goal of this project is simple: understand how containers work by building the core pieces directly with Linux primitives instead of relying on higher-level tooling.

## What It Does

- takes a command from the CLI
- creates an isolated container process with `clone()`
- uses namespaces for hostname, PID, mounts, and networking
- builds a minimal root filesystem with OverlayFS
- switches the container into that rootfs with `pivot_root`
- mounts its own `/proc`
- sets up networking with a veth pair, bridge, routing, and NAT
- applies cgroup v2 limits for memory and process count
- runs the target command with `execvp()`
- cleans up temporary filesystem and network state after exit

## What It Is Not

Aegis is a learning project, not a production-ready container engine. It is intentionally small and focused on the basic runtime flow.

## Run

You’ll need Linux and root privileges for namespace, mount, network, and cgroup operations.

Example:

```bash
sudo ./aegis /bin/bash
```

or

```bash
sudo ./aegis /bin/ping google.com
```

## Notes

- the container filesystem is assembled under `/tmp/aegis`
- only a small set of binaries is copied into the container rootfs
- the current resource limits are 3 processes and 200 MB of memory
