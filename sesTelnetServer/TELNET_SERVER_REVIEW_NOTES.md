# Telnet Server Review Notes

Date: 2026-05-18
Scope: Core Telnet server, transport abstraction, AT command parser integration, and project configuration/docs.

## High Priority Findings

1. Potential buffer overrun in AT command line reader
- Location: sesATParser.cpp:248, sesATParser.cpp:269
- Issue: `readCmdLine()` allows writes while `cmdLen < len`, then always writes a null terminator at `cmdLine[cmdLen]` on CR. If `cmdLen` reaches `len`, the terminator write is out of bounds.
- Impact: Memory corruption risk when command mode is used (`+++` path), including undefined behavior and hard-to-debug crashes.
- Improvement:
  - Change capacity check to `cmdLen < (len - 1)`.
  - Keep explicit room for `\0` terminator.
  - Add a small unit-style test harness (or serial test script) for boundary lengths (0, 1, exact full buffer, overflow).

2. Tight busy-wait loops can starve background work (especially WiFi stacks)
- Location: sesTelnetServer.cpp:246, sesTelnetServer.cpp:322, sesTelnetServer.cpp:428
- Issue: Several loops spin continuously with no `delay()`, `yield()`, or scheduler-friendly pause when no input/connection is available.
- Impact: High CPU usage and possible starvation of networking/RTOS background maintenance on some platforms; reduced responsiveness for other tasks.
- Improvement:
  - Add a tiny cooperative pause in idle paths (`yield()` or short `delay(1)`).
  - Consider restructuring to non-blocking poll functions called from `loop()` rather than long blocking waits.

3. Connection/init paths can block forever with no timeout or recovery strategy
- Location: sesNetwork.cpp:94, sesNetwork.cpp:105, sesTelnetServer.cpp:428
- Issue:
  - WiFi module missing path halts forever (`while (true);`).
  - WiFi connect retries forever with fixed 10s waits.
  - Telnet wait-for-connection loop is indefinite.
- Impact: Device can become permanently unavailable without watchdog reset or user intervention.
- Improvement:
  - Add retry budgets and timeout returns to caller.
  - Surface status/error codes so the sketch can decide fallback behavior.
  - Optional exponential backoff for WiFi reconnect attempts.

## Medium Priority Findings

4. Non-portable AVR header included in transport-agnostic code paths
- Location: sesTelnetServer.cpp:58, sesTelnetServerDebug.cpp:34
- Issue: `<avr/pgmspace.h>` is included directly even though project now targets WiFi S3 / non-AVR boards.
- Impact: Portability/build risk across cores that do not expose AVR headers the same way.
- Improvement:
  - Route flash/progmem access exclusively through `FlashMem.h` abstraction.
  - If direct access is required, gate AVR-specific includes with architecture checks.

5. Parser mode state persists across sessions and is not reset on disconnect/new client
- Location: sesTelnetServer.cpp:494
- Issue: `telnetRead()` parser mode is `static`, so a partial Telnet command at disconnect can influence parsing for next session.
- Impact: Rare but real protocol desynchronization on reconnect.
- Improvement:
  - Reset parser mode on new connection/disconnect, or move parser state into a per-session struct.

6. Printable character filter allows 128 and excludes canonical ASCII boundary intent
- Location: sesTelnetServer.cpp:377
- Issue: Current check uses `(ch >= 32) && (ch <= 128)`. ASCII printable range is typically 32..126.
- Impact: May pass through a non-printable extended value depending on client encoding.
- Improvement:
  - Use 32..126 (or explicit policy for extended character sets).

7. Public header uses Arduino `boolean` while implementation uses `bool`
- Location: sesTelnetServer.h:136, sesTelnetServer.h:137, sesTelnetServer.cpp:74, sesTelnetServer.cpp:75
- Issue: Type mismatch in interface vs implementation style.
- Impact: Usually compiles due to Arduino aliases, but reduces clarity/consistency and portability.
- Improvement:
  - Standardize on `bool` in headers and source.

## Low Priority / Cleanup

8. Default configuration enables verbose Telnet debug
- Location: sesTelnetServerConfig.h:41
- Issue: `TELNET_DEBUG` is enabled by default while comments describe production use with debug off.
- Impact: Increased flash/RAM usage and serial noise in normal builds.
- Improvement:
  - Default this to off and document how to enable for troubleshooting.

9. Documentation/config drift
- Location: README.md:41, README.md:44, README.md:48
- Issue: README references symbols/settings that no longer match current config naming (for example WiFi credential macro names and legacy config examples).
- Impact: Higher setup friction and misconfiguration risk for users.
- Improvement:
  - Update README snippets to match current `sesTelnetServerConfig.h` and `arduino_secrets.h` conventions.

10. Minor comment accuracy issue
- Location: sesTelnetServer.cpp:456, sesTelnetServer.cpp:462
- Issue: `delay(100)` is annotated as "half second pause".
- Impact: Small maintenance confusion.
- Improvement:
  - Correct comment or delay value.

## Suggested Improvement Plan

1. Fix safety issues first
- Patch `readCmdLine()` bounds handling.
- Add parser-state reset on connect/disconnect.

2. Improve runtime robustness
- Add cooperative idle yields in polling loops.
- Add timeouts/retry ceilings for WiFi and connection wait loops.

3. Clean up portability and maintainability
- Remove unconditional AVR includes from non-AVR paths.
- Normalize bool types in public headers.
- Align docs/examples with current configuration.

## Test Ideas After Changes

- Fuzz-like Telnet negotiation stream tests:
  - Repeated `IAC` sequences.
  - Interrupted subnegotiation sequences across disconnect/reconnect.
- Boundary input tests for command buffers:
  - Exact-fit commands and overlong commands in both Telnet and AT command modes.
- Soak tests:
  - Long idle periods with WiFi connected.
  - Repeated connect/disconnect cycles from multiple clients.
