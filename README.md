<div align="center">

# ft_ping

**A from-scratch reimplementation of the Unix `ping` utility in C.**

Built against the behavior and output format of `ping (GNU inetutils) 2.0`.

![Language](https://img.shields.io/badge/language-C-00599C?style=flat-square&logo=c)
![Build](https://img.shields.io/badge/build-Makefile-427819?style=flat-square&logo=gnu)
![Standard](https://img.shields.io/badge/standard-C99-blue?style=flat-square)
![Norm](https://img.shields.io/badge/CFLAGS-Wall%20Wextra%20Werror-critical?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
![42](https://img.shields.io/badge/school-42-000000?style=flat-square)

</div>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Build](#build)
- [Usage](#usage)
- [Options](#options)
- [Examples](#examples)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Technical Notes](#technical-notes)
- [Reference](#reference)
- [Author](#author)

---

## Overview

`ft_ping` sends ICMP `ECHO_REQUEST` packets to a network host and reports
round-trip times, TTL, packet loss and aggregated statistics. It is a
faithful reproduction of the reference `inetutils-2.0` `ping`, written
without using its source as a dependency.

The program operates on a **raw IPv4 socket** (`SOCK_RAW`, `IPPROTO_ICMP`),
crafts the ICMP header by hand (including the Internet checksum), and
parses the kernel-supplied IPv4 + ICMP reply headers manually.

> **Note:** raw sockets require elevated privileges. Run with `sudo` or grant
> the `cap_net_raw` capability to the binary.

---

## Features

- IPv4 ICMP Echo Request / Echo Reply
- DNS resolution (`getaddrinfo`)
- Per-reply RTT measurement (timestamp embedded in the payload)
- Final statistics: `min` / `avg` / `max` / `mdev` (stddev) and packet loss
- ICMP error decoding (`Destination Unreachable`, `Time Exceeded`,
  `Redirect`, `Source Quench`, `Parameter Problem`)
- Verbose mode with full IP-header dump of the embedded original packet
- Drift-free send loop (absolute scheduling, no cumulative jitter)
- Defensive parsing: bounded numeric ranges, signed/unsigned overflow
  guards, strict numeric validation
- Zero memory leaks (verified with valgrind)
- Compiled with `-Wall -Wextra -Werror`

---

## Build

```bash
make           # build ./ft_ping
make clean     # remove object files
make fclean    # remove objects + binary
make re        # full rebuild
```

**Requirements:**
- `gcc` (or any C99-compatible compiler)
- GNU `make`
- A Linux kernel with raw socket support
- `libm` (linked automatically via `-lm`)

---

## Usage

```bash
sudo ./ft_ping [OPTIONS] HOST
```

`HOST` may be either an IPv4 address (`8.8.8.8`) or a DNS name
(`google.com`).

---

## Options

| Flag        | Argument  | Range            | Description                                              |
|:------------|:----------|:-----------------|:---------------------------------------------------------|
| `-v`        | —         | —                | Verbose output (PID line, ICMP error dumps)              |
| `-q`        | —         | —                | Quiet output (statistics only)                           |
| `-c`        | `NUMBER`  | `0` = infinite   | Stop after `NUMBER` packets sent                         |
| `-w`        | `NUMBER`  | `1 .. INT_MAX`   | Global deadline in seconds                               |
| `-W`        | `NUMBER`  | `1 .. INT_MAX`   | Per-reply timeout in seconds (default `1`)               |
| `-s`        | `NUMBER`  | `0 .. 65399`     | Payload size in bytes (default `56`)                     |
| `-r`        | —         | —                | Bypass routing table (`SO_DONTROUTE`) — local-link only  |
| `--ttl`     | `N`       | `1 .. 255`       | Set IP Time-To-Live (default `64`)                       |
| `-?`        | —         | —                | Display the help list                                    |
| `-V`        | —         | —                | Print program version                                    |

---

## Examples

```bash
# Continuous ping of a public DNS resolver
sudo ./ft_ping 1.1.1.1

# Send exactly 5 packets
sudo ./ft_ping -c 5 google.com

# 10-second deadline with a tight 1-second per-reply timeout
sudo ./ft_ping -w 10 -W 1 8.8.8.8

# Large payload (1500 bytes) and shortened TTL
sudo ./ft_ping -s 1500 --ttl=8 example.com

# Verbose mode: surface ICMP errors with full IP header dump
sudo ./ft_ping -v -c 3 240.0.0.1

# Quiet mode: only the final statistics block
sudo ./ft_ping -q -c 100 1.1.1.1
```

---

## Architecture

```
                  ┌─────────────┐
                  │   main.c    │  argv → flags, hostname → IP
                  └──────┬──────┘
                         │
              ┌──────────┴──────────┐
              ▼                     ▼
        ┌──────────┐          ┌───────────┐
        │ parsing  │          │  socket   │  raw IPv4 / ICMP
        │   .c     │          │    .c     │  TTL + SO_RCVTIMEO
        └──────────┘          └─────┬─────┘
                                    │
                                    ▼
                              ┌───────────┐
                ┌────────────►│  icmp.c   │  send loop, timing
                │             └─────┬─────┘  & deadline control
                │                   │
        ┌───────┴────┐              ▼
        │  packet.c  │        ┌─────────────┐
        │ (build +   │        │ icmp_reply  │  recvfrom → parse
        │  checksum) │        │     .c      │  → validate → stats
        └────────────┘        └─────┬───────┘
                                    │
                                    ▼
                              ┌──────────┐
                              │  logs.c  │  display + final stats
                              └──────────┘
```

---

## Project Structure

```
ft_Ping_42/
├── Makefile               # -Wall -Wextra -Werror, links -lm
├── ping.h                 # public types, constants, prototypes
├── main.c                 # entry point, hostname resolution
├── parsing.c              # CLI parsing + numeric validation
├── handle_flag.c          # -? and -V handlers
├── socket.c               # raw socket setup, TTL, timeouts
├── packet.c               # ICMP header + payload + checksum
├── icmp.c                 # send loop, deadline-aware sleep
├── icmp_reply.c           # recvfrom, parse, validate, stats
├── logs.c                 # per-reply + verbose + final output
├── utils.c                # ft_isnumber
├── DOC/                   # study notes (FR) on ICMP, sockets, RTT...
└── train/                 # standalone training programs
```

---

## Technical Notes

**ICMP packet construction.** The kernel writes the IPv4 header for us
(`SOCK_RAW` at L3), so the program builds only the ICMP header
(`type`, `code`, `id` = `pid & 0xFFFF`, `sequence`) followed by the
payload. The first 16 bytes of the payload hold a `struct timeval`
captured at send time, which lets the receiver compute the RTT without
keeping per-sequence state.

**Internet checksum.** The 16-bit one's complement sum is folded with a
double-fold to absorb any carry generated by the first fold, then bitwise
inverted — exactly as specified in RFC 1071.

**Timing.** The send loop schedules each packet against an absolute
reference (`start + n * interval`) instead of `sleep(1)` between sends,
so no cumulative drift accumulates over long sessions. When `-w` is
active, `SO_RCVTIMEO` is dynamically capped to the remaining deadline,
which prevents the final `recvfrom` from outliving the deadline window.

**Error handling.** ICMP errors that arrive on the same socket are
matched against the embedded original packet (IP header + 8 transport
bytes). With `-v`, the program reproduces the exact `inetutils` dump
layout, decoding `Vr HL TOS Len ID Flg off TTL Pro cks Src Dst`.

**Statistics.** RTT `mdev` is computed from a streaming
sum and sum-of-squares — `variance = E[X²] − E[X]²` — clamped to `0` to
guard against floating-point negatives on near-constant samples.

---

## Reference

- [RFC 792 — Internet Control Message Protocol](https://www.rfc-editor.org/rfc/rfc792)
- [RFC 1071 — Computing the Internet Checksum](https://www.rfc-editor.org/rfc/rfc1071)
- [GNU inetutils](https://www.gnu.org/software/inetutils/) — reference implementation
- `man 7 raw`, `man 7 icmp`, `man 2 socket`, `man 2 setsockopt`

---

## Author

**throbert** — 42 School project (Network 1 / `ft_ping`).
