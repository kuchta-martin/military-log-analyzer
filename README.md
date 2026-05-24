# Military Base Log Analyzer

[![Build](https://github.com/kuchta-martin/military-log-analyzer/actions/workflows/build.yml/badge.svg)](https://github.com/kuchta-martin/military-log-analyzer/actions/workflows/build.yml)

A C++20 command-line tool that reconstructs **who could have visited a restricted zone** inside a military base, given access logs and the base's zone-connectivity graph.

Developed as a semester project for [PA2 – Programming and Algorithmics 2](https://fit.cvut.cz/) at FIT CTU Prague.  
**Progtest score: 6.60 / 6.00** (above-maximum bonus points awarded).

---

## The Problem

A military base is divided into named **zones** connected by corridors with known travel times (in minutes).  
Entry/exit events are recorded in log files in three formats: plain-text, little-endian binary, and big-endian binary.

Given a target zone and an optional time window, find every person who **could have physically reached** that zone — accounting for travel time from their actual entry/exit points.

### Why this is non-trivial

A person is a suspect only if there exists an entry/exit pair where:

```
entry_time + dist(entry_zone → target) ≤ window_end
exit_time  − dist(exit_zone  → target) ≥ window_start
```

Both constraints must hold simultaneously, which requires knowing shortest paths between every pair of zones.

---

## Design

```
include/
  date_utils.hpp      – leap-year math, toMinutes(), LogEntry struct
  audit_filter.hpp    – CAuditFilter  (fluent builder: zone + time window)
  visitor_log.hpp     – CVisitorLog   (immutable query snapshot)
  mil_base.hpp        – CMilBase      (graph + log parser)
src/
  visitor_log.cpp     – search() implementation
  mil_base.cpp        – BFS, binary I/O, text parser
  main.cpp            – demo using examples/
```

### Key design decisions

| Decision | Rationale |
|---|---|
| All-pairs BFS at load time | Queries are O(people × visits); pre-computing distances makes each lookup O(1) |
| `CVisitorLog` is immutable | Constructed once by `CMilBase::processLog()` (friend), then only read |
| Template `processBinaryBlock<ReadU16, ReadU32>` | Same parsing logic for LE and BE byte orders — zero code duplication |
| `std::size_t` zone indices throughout | Eliminates signed/unsigned mismatch warnings under `-Wconversion` |
| No raw `new` / `delete` | RAII via `std::vector`, `std::map`, `std::string` |

---

## Build

Requires a C++20 compiler and CMake ≥ 3.16.

```bash
cmake -S . -B build
cmake --build build
```

Compiled with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` — **zero warnings**.

### Manual (no CMake)

```bash
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Iinclude \
        src/main.cpp src/mil_base.cpp src/visitor_log.cpp -o mil-analyzer
```

---

## Running

```bash
./build/mil-analyzer                          # uses examples/ by default
./build/mil-analyzer base.txt access.log      # custom files
```

Expected output with the bundled sample data:

```
Who could have visited 'armory' (any time)?
  · Alice Cooper
  · Bob Smith
  · Carol White

Who could have visited 'armory' between 08:20-08:30 on 2026-03-10?
  · Alice Cooper
  · Bob Smith

Who could have visited 'lab' (any time)?
  · Alice Cooper
  · Bob Smith
  · Carol White
```

See [`examples/README.md`](examples/README.md) for a walkthrough of how each result is derived.

---

## Usage (API)

```cpp
CMilBase base;
base.readBase("examples/base.txt");      // load zone graph

CVisitorLog log = base.processLog("examples/sample.log");  // parse log

// Who was near the armory on the morning of 10 March 2026?
std::set<std::string> suspects =
    log.search(CAuditFilter("armory")
                   .notBefore(2026, 3, 10,  8, 0)
                   .notAfter (2026, 3, 10, 12, 0));
```

### Log file format

`processLog` auto-detects the block type from a 4-byte magic header:

| Magic | Format |
|---|---|
| `TEXT` | Plain-text: zone name + count, then `YYYY-MM-DD HH:MM name` lines |
| `IIII` | Little-endian binary (packed 32-bit datetime + length-prefixed strings) |
| `MMMM` | Big-endian binary (same layout, reversed byte order) |

A single log file may interleave all three block types.

---

## Datetime encoding (binary blocks)

32-bit packed field — bit layout:

```
 31      20 19   16 15  11 10   6 5     0
 YYYYYYYYYYYY MMMM DDDDD HHHHH IIIIII
```

Fields: year (12 bits) · month (4) · day (5) · hour (5) · minute (6).

---

## Author

Martin Kuchta — FIT CTU Prague, 1st year  
[github.com/kuchta-martin](https://github.com/kuchta-martin)
