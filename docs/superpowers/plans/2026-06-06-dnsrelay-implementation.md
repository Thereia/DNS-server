# DNSRelay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the DNS relay incrementally, starting from a minimal UDP listener and expanding toward local table lookup, DNS parsing, direct replies, upstream relay, ID mapping, and timeout cleanup.

**Architecture:** Use a single-threaded UDP event loop. Keep local rule loading, DNS packet parsing, relay state, and timeout cleanup split into small modules only when they become necessary.

**Tech Stack:** Visual Studio, C, Winsock2, PowerShell smoke test, small local test helpers

---

## Current Progress

- [x] Stage 1: Minimal UDP listener on `127.0.0.1:5533`
- [x] Minimal UDP sender utility
- [x] Smoke test that proves packet receipt

## Next Steps

### Task 2: Local Rule Table

- Add `dnsrelay.txt` loading
- Parse `IP domain` pairs
- Support local lookup by queried domain

### Task 3: DNS Request Parsing

- Parse request header
- Decode `QNAME`
- Extract `ID`, `QTYPE`, `QCLASS`

### Task 4: Local Direct Responses

- Return configured A records directly
- Return NXDOMAIN-style response for `0.0.0.0`

### Task 5: Upstream Relay

- Forward misses to upstream DNS
- Translate IDs
- Map responses back to the original client

### Task 6: Timeout Cleanup

- Remove expired in-flight relay records
- Add basic debug output modes
