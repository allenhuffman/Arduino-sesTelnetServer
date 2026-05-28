# sesATParser Review Notes

Date: 2026-05-18
Scope: `sesATParser.h` and `sesATParser.cpp`

## High Priority Findings

1. Command mode can trap forever with no escape path besides recognized commands
- Location: `sesATParser.cpp` (`cmdMode`, `while (1)`)
- Issue: `cmdMode()` loops forever and only exits on exact `ATO`.
- Impact: If line handling or command recognition fails, local console can remain stuck in command mode.
- Improvement:
  - Add `ATZ` and/or `EXIT` aliases to leave mode.
  - Add optional timeout/inactivity exit in test builds.

## Medium Priority Findings

2. Busy wait in command input loop can starve other background work
- Location: `sesATParser.cpp` (`readCmdLine`, `while (!done)`)
- Issue: Loop spins until CR with no `yield()`/short delay when no serial data is present.
- Impact: Increased CPU usage and possible starvation on cores where background work is cooperative.
- Improvement:
  - Add `yield()` in no-data path, or `delay(1)` when appropriate for the target.

3. End-of-line handling is CR-only and may mishandle CRLF/LF-only terminals
- Location: `sesATParser.cpp` (`readCmdLine`, switch on `ch`)
- Issue: `CR` ends the line; `LF` is treated as a regular non-printable char and ignored.
- Impact: Works in many cases, but can leave residual LF bytes that can cause surprising follow-up behavior depending on caller flow.
- Improvement:
  - Explicitly ignore LF as line terminator tail after CR.
  - Optionally accept LF-only as line termination for more terminal compatibility.

4. Command matching policy is mixed (exact vs prefix) and not documented
- Location: `sesATParser.cpp` (`cmdMode`)
- Issue: `ATD` uses prefix match (`strncmp`), while `AT`/`ATO` use exact match.
- Impact: Easy to introduce ambiguous command behavior as parser grows.
- Improvement:
  - Document command grammar (exact vs prefix rules).
  - Centralize matching in helper functions.

## Low Priority Findings

5. Unused macros create noise and maintenance risk
- Location: `sesATParser.cpp` (`INBUF_SIZE`, `OUTBUF_SIZE`)
- Issue: Macros are defined but not used.
- Impact: Confusing for maintenance and code review.
- Improvement:
  - Remove unused macros or wire them into actual buffer sizing.

6. Comment unit mismatch for guard time
- Location: `sesATParser.cpp` (`ESC_GUARD_TIME` comment)
- Issue: Comment says "Seconds required" but value `1000` is used with `millis()` (milliseconds).
- Impact: Misleads maintainers and test tuning.
- Improvement:
  - Update comment to milliseconds, or rename macro to include unit (`ESC_GUARD_TIME_MS`).

7. Help text and supported commands can drift
- Location: `sesATParser.cpp` (`cmdMode` help output)
- Issue: Static help text may not stay aligned with command table over time.
- Impact: Confusion for users/testing.
- Improvement:
  - Keep help text generated from a single command table, or review whenever command logic changes.

## Positive Notes

1. Buffer overrun risk in command line input has been fixed
- `readCmdLine()` correctly keeps room for null terminator with `cmdLen < (len - 1U)`.

2. Escape-sequence state machine split is clearer now
- `cmdModeFeed()` (feed bytes) and `cmdModeCheck()` (poll for completion) improves readability and usage intent.

## Suggested Next Steps

1. Add explicit LF handling policy in `readCmdLine()`.
2. Add cooperative yielding in `readCmdLine()` idle path.
3. Define and document command grammar (exact/prefix) and optional escape hatches (`EXIT`/`ATZ`).
4. Clean up unused macros and comment units.
