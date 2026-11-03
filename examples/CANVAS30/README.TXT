CANVAS30 - moving-marker example (Windows 3.0 real mode / 8088)

Link the optional libtandywin/CANVAS/WTCANVAS.C, not LTWUI.C. The source uses a
maximum 160x96 monochrome surface anchored at client (0,0). Resize recreates a
smaller/larger bounded surface; unused client space stays black. Minimize
pauses movement. Exposes copy initialized retained pixels using BeginPaint's
clip; pending animation damage is included before paint and only committed
after successful drawing/copy. Failed draws force full retry on the next timer.

Copy CANVAS.C, CANVAS.DEF, CANVAS.RSP, BUILD.BAT and WTCANVAS.C/H into a fresh
private staging directory. With caller-owned MSC6, LINK4 and Windows SDK
PATH/INCLUDE/LIB configured, run BUILD.BAT. SLIBCEW/LIBW must be provided by
the caller's SDK; LINK supplies its default DOS stub. /G0 /Gw /AS targets 8086 Windows small model.
The helper and consumer must use the same memory model. No SDK is bundled.

Run only in an authorized, isolated Windows 3.0 test guest. Check motion,
cover/uncover, repeated minimize/restore, narrow/short windows and repeated
resize/open/close. A host or compile-only check is not a native pixel or timing
qualification. This example is deliberately not a saver, Cube port, widget
framework or display-driver change.

Initial placement caps the outer frame to valid screen dimensions and uses
origin (0,0) if either dimension needs capping. Larger screens retain the
220x150 default USER placement. The canvas limit stays 160x96 client pixels.

Partial native validation (2026-10-30) runs the actual overlapped WinMain and
message loop as a separate task on Windows 3.0 real mode, Tandy HX 8088/640 KB,
with exact current TR51616 and TR53216 drivers. Native timer motion, maximize,
minimize, restore to remembered maximized state, a second restore to normal,
resize, overlap/expose, system Close, relaunch and headless keyboard Alt-F4
passed. Two launches/closes per run return to identical free memory and unload
the task. The original compact-screen frame clipped at 168 pixels; the fixed
160-column normal frame is exactly 160 pixels and fully visible.

See ../../libtandywin/CANVAS/NATIVE.JSN for exact runtime, driver, footprint
and test limits. These are bounded emulator tests; physical mouse dragging,
long-duration exhaustion, other native modes and a runtime distribution are
not qualified. Source is not a published binary package.

FAILURE OWNERSHIP / FULL-CLIENT RETRY
WinMain reserves one application-owned 100 ms task timer before creating the
window. It drives animation through the message loop; the helper owns no
timer. After WM_DESTROY, a failed canvas deletion leaves the task and timer
alive. Later ticks retry without a busy loop. Quit is posted and the timer is
cancelled only after all owned canvas handles are released. A persistent
deletion failure deliberately retains the task and ownership for later retry.

Background FillRect results are part of paint success. A background, draw or
copy failure requests a full-client retry before BeginPaint; marker-only
damage or unrelated exposure cannot discard a failed outside-canvas region.
A successful paint clears the retry state and error caption.

1485 host example assertions and normal/injected actual-WinMain runs in both
matched TR5 modes qualify this sample change. Native API wrappers partially
paint a background strip and fail, then fail DC and bitmap deletion on close.
They verify full-client black restoration and quit only after both handles
release. See ../../libtandywin/CANVAS/FAILURES.JSN for runtime identities,
footprint, exact scope and limits. Helper C/H remain unchanged.
