# Development attempts, not acceptance passes

FMT01: 14 failures in 65 checks. The burst-only short-gap fallback delayed
service; corrected in the next build. FMT02: 7 failures in 68 checks. The
128-group reserve starved during foreground work; replaced by 512 groups and
bounded refill without shifting timestamps. FMT03 used the final BUILD04
runtime but its combined harness hit the 280-second watchdog before FINAL.
It is incomplete, not a pass. The independent split cases in TESTS/NATIVE
are the acceptance results. Old failing runtime binaries are not distributed.
SOURCE/FMTGUI.* retains the historical combined harness needed by the saved
BUILD04 batch; the consolidated current recipe uses the split harnesses.
