# Aegis

Aegis is a lightweight Linux container runtime prototype written in C++. It builds a restricted execution environment by combining Linux namespaces, `chroot`, a private `proc` mount, and cgroup-based resource limits. The current codebase is intentionally small, but it already shows the core control flow behind a basic container launcher.

This project is closer to a learning-focused runtime skeleton than a production container engine. It demonstrates the mechanics of process isolation directly through low-level syscalls instead of hiding them behind higher-level tooling.

### At a high level, Aegis:

- accepts a command from the CLI
- creates a child process with isolated namespaces using `clone(2)`
- switches that child into a dedicated root filesystem
- mounts a fresh `proc` filesystem inside the new mount namespace
- applies process-count and memory limits through cgroups
- executes the requested program inside the isolated environment

## Architecture Overview

The runtime flow is straightforward:

1. `main()` collects the command arguments passed to Aegis.
2. The command is converted from `std::vector<std::string>` into the `char*[]` shape expected by `execvp`.
3. A manual stack buffer is allocated for `clone`.
4. `clone` starts a child in new UTS, PID, and mount namespaces.
5. The parent places that child in dedicated cgroups and waits.
6. Inside the cloned context, `runtime()`:
   - makes mounts private
   - sets the hostname to `Aegis`
   - `chroot`s into `ubuntu-file-system`
   - mounts `proc`
   - forks once more so PID 1 exists inside the new PID namespace
   - `execvp`s the requested command
7. When the command exits, the temporary `proc` mount is cleaned up.

That combination gives Aegis basic filesystem, hostname, PID-space, and mount isolation, with minimal resource controls.

## Technical Deep Dive

### 1. Namespace Creation

The core isolation boundary is created with `clone(...)`. The code uses:

- `CLONE_NEWUTS` to isolate the hostname
- `CLONE_NEWPID` to create a fresh PID namespace
- `CLONE_NEWNS` to isolate mount changes
- `SIGCHLD` so the parent can wait for child termination normally

This is the main reason Aegis feels like a container runtime instead of a plain process launcher.

### 2. Root Filesystem Switching

Inside `runtime()`, the child calls `chroot("/home/karthik/Projects/Aegis/ubuntu-file-system")`, then switches to `/` with `chdir("/")`.

That means commands run against the bundled Ubuntu-style rootfs rather than the host filesystem. In practice, Aegis depends on that rootfs containing enough binaries, libraries, device nodes, and configuration for the target process to start successfully.

### 3. Mount Isolation

Before remounting anything, Aegis marks the mount tree private with:

- `mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)`

Without this step, mount events could propagate back to the host or other namespaces depending on the host mount configuration.

After entering the new rootfs, it mounts:

- `proc` at `/proc` inside the container namespace

That mount is created in [`main.cpp:54`](./main.cpp#L54) and removed during cleanup in [`main.cpp:64`](./main.cpp#L64).

### 4. PID Namespace Behavior

Because `CLONE_NEWPID` only affects children created after the namespace is established, Aegis performs an extra `fork()` inside `runtime()`. The forked child becomes the process that actually runs the target command, while the immediate parent in the namespace acts as the namespace init-like process that waits and cleans up.

This is an important implementation detail. Without the extra `fork`, the executed command would not occupy the intended PID position within the new namespace.

### 5. Resource Controls with Cgroups

The `rule_set()` function creates two cgroup directories:

- `/sys/fs/cgroup/pids/Aegis`
- `/sys/fs/cgroup/memory/Aegis`

It then writes the cloned child into each cgroup and applies:

- `pids.max = 3`
- `memory.limit_in_bytes = 209715200` (200 MiB)

This gives Aegis a simple but useful resource envelope: the container cannot spawn more than a few processes and is capped at roughly 200 MiB of memory.

The current implementation assumes a cgroup v1-style layout. On machines using unified cgroup v2 only, these paths and control files will need to change.

### 6. Command Execution Model

The final program launch uses `execvp()`. This replaces the forked child process image with the requested command. Aegis therefore acts as the launcher and supervisor, but not as a shell.

That means:

- argument parsing is simple
- shell features such as pipes or redirection are not interpreted by Aegis itself
- the executable must exist inside the container rootfs and be discoverable via its `PATH`

## Build

The code currently builds as a single C++ source file:

```bash
g++ -std=c++17 -o bin main.cpp
```

This compile path was verified against the current source.

## Running

Aegis depends on Linux kernel features that typically require root privileges or equivalent capabilities:

- namespaces
- `chroot`
- mounting `proc`
- writing to cgroup control files

Example usage:

```bash
sudo ./bin /bin/bash
```

or:

```bash
sudo ./bin /bin/echo hello
```

For this to work, `ubuntu-file-system/` must contain the matching binaries and libraries expected by the command you run.

## Project Structure

- `main.cpp`: runtime implementation
- `ubuntu-file-system/`: root filesystem used for `chroot`
