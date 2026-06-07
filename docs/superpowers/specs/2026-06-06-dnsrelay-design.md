# DNSRelay Design

**Goal**

Build a course-project DNS relay server for Windows using C and Winsock. The program should read a local `dnsrelay.txt` rule table, answer local hits directly, return "domain not found" for blocked domains mapped to `0.0.0.0`, and relay misses to an upstream DNS server with request ID translation and timeout cleanup.

**Environment**

- OS: Windows
- Toolchain: Visual Studio
- Language: C
- Network stack: Winsock

## Scope

The implementation target is a practical, course-appropriate DNS relay server:

- UDP-based server
- Local rule-table lookup
- Upstream DNS forwarding
- Request/response ID mapping
- Timeout cleanup
- Debug output levels similar to `-d` and `-dd`

Out of scope for the first complete version:

- Multithreaded architecture
- Full recursive resolver behavior
- Support for many record types beyond the minimum needed for the assignment
- Performance tuning or production-grade caching

## Recommended Architecture

Use a single-threaded event-loop design.

Why this design:

- It matches the assignment well
- It keeps the control flow understandable
- It is enough to support multiple in-flight requests through an ID mapping table
- It avoids thread-synchronization complexity early on

## Runtime Data Flow

1. Program starts
2. Load `dnsrelay.txt`
3. Initialize Winsock
4. Create UDP socket and bind to the configured local port
5. Receive a DNS request from a client
6. Parse the request header and question
7. Extract the query domain and query type
8. Lookup the domain in the local rule table
9. Handle one of three cases:
   - Local IP hit: build and send a direct response
   - `0.0.0.0` hit: build and send an NXDOMAIN-style response
   - Miss: allocate a relay ID, record mapping, forward to upstream DNS
10. Receive upstream response
11. Restore the original client ID using the relay table
12. Send the restored response back to the client
13. Periodically remove timed-out relay records

## Module Layout

Recommended project layout:

```text
dnsrelay/
├─ dnsrelay.sln
├─ dnsrelay/
│  ├─ main.c
│  ├─ common.h
│  ├─ server.c
│  ├─ server.h
│  ├─ hosts.c
│  ├─ hosts.h
│  ├─ dns_packet.c
│  ├─ dns_packet.h
│  ├─ relay_table.c
│  ├─ relay_table.h
│  ├─ timeout.c
│  ├─ timeout.h
│  └─ dnsrelay.txt
└─ tools/
   └─ udp_sender.c
```

## Module Responsibilities

### `main.c`

- Program entry point
- Parse command-line arguments
- Initialize Winsock
- Load rule table
- Create server state
- Start the main receive loop

### `common.h`

- Global constants
- Shared small types
- Buffer sizes
- Port defaults
- Timeout defaults

### `server.c` / `server.h`

- Own the UDP socket
- Bind to the listening address and port
- Receive packets
- Distinguish between client requests and upstream responses
- Dispatch handling to packet/rule/relay modules

### `hosts.c` / `hosts.h`

- Load `dnsrelay.txt`
- Store `domain -> ip` mappings in memory
- Provide lookup by domain

### `dns_packet.c` / `dns_packet.h`

- Parse DNS header and question section
- Decode `QNAME`
- Read `ID`, `QTYPE`, `QCLASS`
- Build direct A-record responses
- Build NXDOMAIN-style responses
- Update packet IDs when relaying

### `relay_table.c` / `relay_table.h`

- Track active forwarded requests
- Store:
  - upstream relay ID
  - original client ID
  - client address/port
  - timestamp
  - active flag
- Lookup by relay ID on upstream response

### `timeout.c` / `timeout.h`

- Remove expired relay mappings
- Support periodic cleanup during the main loop

### `tools/udp_sender.c`

- Minimal UDP test sender for early development
- Used only to verify that the local UDP listener can receive packets before full DNS support is implemented

## Core Data Structures

### Host Rule Record

Minimum fields:

- domain string
- IPv4 address
- blocked flag or equivalent interpretation of `0.0.0.0`

Simple array-based storage is acceptable for the course project at first.

### Relay Record

Minimum fields:

- upstream relay ID
- original client ID
- client socket address
- send timestamp
- active flag

### Packet Buffer

Use fixed-size receive/send buffers.

Minimum practical assumption:

- start with a safe fixed buffer for UDP packets
- support standard DNS packet sizes used by the assignment

## Development Stages

### Stage 1: UDP Listener Skeleton

Purpose:

- Verify the program can initialize Winsock
- Create a UDP socket
- Bind to `127.0.0.1:5353`
- Receive a UDP packet
- Print source IP, source port, and payload length

This stage intentionally avoids real DNS logic.

### Stage 2: Rule Table Loading

Purpose:

- Read `dnsrelay.txt`
- Parse lines of the form `IP domain`
- Support simple lookup by domain

This can be tested independently from real DNS traffic.

### Stage 3: DNS Request Parsing

Purpose:

- Parse the incoming DNS request
- Extract:
  - `ID`
  - queried domain
  - query type

After this stage the program can understand what the client is asking.

### Stage 4: Local Direct Responses

Purpose:

- If the domain maps to a normal IP, return a direct answer
- If the domain maps to `0.0.0.0`, return an NXDOMAIN-style response

After this stage the server works as a local DNS responder for configured rules.

### Stage 5: Upstream Relay + ID Translation

Purpose:

- Forward local misses to upstream DNS
- Replace the original client ID with a relay ID
- Save mapping state
- Restore the original ID when the upstream response returns
- Send the response back to the original client

This stage implements the core relay behavior required by the assignment.

### Stage 6: Timeout Cleanup + Debug Modes

Purpose:

- Remove stale relay entries
- Handle no-response and late-response cases safely
- Add debug output modes

This stage makes the project more robust and ready for demonstration.

## Testing Strategy

### Early Testing

- Use `tools/udp_sender.c` to verify packet receipt on `127.0.0.1:5353`

### Mid-Stage Testing

- Feed known domains through the local rule table
- Confirm direct-hit and blocked-domain behavior

### End-to-End Testing

- Point local DNS to the program
- Use `nslookup`
- Use `ping`
- Verify local hit, blocked result, and relay miss behavior

### Validation Focus

The final validation should explicitly prove:

- local rule hit works
- `0.0.0.0` block works
- upstream relay works
- concurrent in-flight requests do not confuse IDs
- timed-out requests are cleaned up safely

## Risks and Constraints

- Binding to port `53` may be harder during early development; use `5353` first, then switch later
- Windows network settings can interfere with full-system DNS testing
- DNS packet parsing is the first protocol-heavy part and should be isolated in its own module
- ID translation is logically required even in a single-threaded loop because multiple requests may be in flight at once

## Recommended First Milestone

The first implementation milestone should be:

`A Visual Studio C program that listens on 127.0.0.1:5353 over UDP and prints received packet metadata.`

This is intentionally small and should be completed before any DNS-specific parsing work begins.
