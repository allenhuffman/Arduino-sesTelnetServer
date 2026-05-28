# telnetInput() Flow

This document describes the runtime flow of `telnetInput()` in
`sesTelnetServer.cpp`.

## Purpose

`telnetInput(char *buffer, uint8_t bufferSize)` reads one command line from:

- Local serial input, or
- Remote Telnet input (when connected and not in offline mode)

It returns when a full line is ready, a connection event occurs, or command
mode transitions out and returns control to the main loop.

## Return Values

- `0`: no command line yet (or command-mode transition path that returns early)
- `1..(bufferSize-1)`: number of bytes in `buffer`
- `255`: remote connection lost

## High-Level Flow

1. Validate arguments
- If `buffer == NULL` or `bufferSize == 0`, function returns `0`.

2. Optional GA signaling
- If not offline, client connected, and `MODE_SUPGA` is not enabled,
  function sends Telnet `GA` (`Go Ahead`).

3. Main read loop (`while done == false`)
- Handles connection state checks.
- Reads from local serial or Telnet.
- Polls for `+++` command-mode completion when idle.
- Processes each input byte into line-edit behavior.

## Detailed Branching

### A. Connection management (non-offline only)

If `offlineMode == false`:

- If `telnetConnected == false`:
  - Calls `telnetWaitForConnection()`.
  - Clears `buffer[0]`.
  - Returns `0` (caller can redraw prompt and loop).

- Else if `client.connected() == false`:
  - Prints connection-lost message.
  - Calls `telnetDisconnect()`.
  - Returns `255`.

### B. Input source selection

Priority is local serial, then Telnet:

1. Local serial available:
- Reads one byte from `Serial`.
- Forces `echoMode = true` for local keystrokes.
- Feeds byte into command-mode detector via `cmdModeFeed(ch)`.

2. Else if Telnet data available and not offline:
- Sets `echoMode` from `MODE_ECHO` negotiation state.
- Reads one Telnet-processed byte via `telnetRead(client)`.

3. Else (no data from either source):
- Polls `cmdModeCheck()` for completed `+++` guard-time sequence.
- If true:
  - Enters `cmdMode()`.
  - Discards queued Telnet RX bytes (if connected) to avoid replaying
    deferred remote typing after resume.
  - Clears current line state (`cmdLen = 0`, `buffer[0] = '\0'`).
  - Breaks out so caller regains control and can redraw prompts.
- If false:
  - Continues loop waiting for input.

## Per-Character Processing

After a byte is acquired, `switch(ch)` handles editing and termination:

- `LF`:
  - Ignored.

- `CR`:
  - If remote echo is active, sends `CRLF` to Telnet client.
  - Prints newline locally.
  - Null-terminates buffer.
  - Marks input done and returns length.

- `CAN` (`^X`):
  - Clears current line buffer.
  - Performs visual erase on both serial and remote (if echo enabled).

- `BS` or `DEL`:
  - Backspace one character if line not empty.
  - Performs visual erase on both serial and remote (if echo enabled).

- Default printable chars:
  - Accepts bytes in range `32..128` (as currently coded).
  - If room remains (`cmdLen < bufferSize - 1`):
    - Optionally echoes to Telnet (if connected and echo enabled).
    - Always echoes to local serial.
    - Appends byte to buffer and increments length.
  - If no room:
    - Rings bell locally and remotely (if connected).

## Notes

- The function is intentionally blocking until a line/event is ready.
- `cmdMode()` transition is integrated so local `+++` can interrupt normal
  line input.
- Prompt printing is handled by the caller (`loop()`), not inside
  `telnetInput()`.
