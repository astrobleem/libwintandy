libwintandy - opt-in bounded GDI canvas and damage helper

Include Windows 3.0 WINDOWS.H then WTCANVAS.H; link WTCANVAS.C once with the
consumer using its existing Win16 memory model. No LTWUI/LWTANDY interface,
DLL export, display driver, timer owner or other consumer is changed.
Production is C89, integer-only, no allocated application buffers or global
state. FAR state/RECT arguments also accept caller stack storage under Win16.

DAMAGE (usable without a bitmap)
  Init(width,height) establishes local coordinates and forces full damage.
  Begin discards only the current-frame union, not the last committed bounds.
  Add each current object's conservative rectangle (including pen/effects).
  Get returns the clipped union of current and previous committed objects.
  Clear that damage to the application's background, redraw every current
  object, and present/invalidate all damage. Commit ONLY after all required
  drawing/presentation succeeds. On partial drawing or an uncertain backing
  surface, Reset forces a full clear/redraw/retry. Do not commit failures.

All rectangles use exclusive right/bottom. Empty, inverted or wholly outside
objects contribute nothing. No padding, geometry, background or occlusion is
inferred. Add every current object, not only objects that moved. No objects
this frame erases the previous union once; after Commit it becomes empty.
Each state is independent. Get is a non-consuming query; Commit is explicit.
Dimensions must be 1..32767. No rectangle arithmetic can wrap a 16-bit int.
Do not mutate fields directly or alias Get's output with the state's fields.

CANVAS (optional)
Zero-initialize LTWCANVAS. Create uses an existing reference DC and explicit
small dimensions, refusing more than 32760 pixels or 32760 word-aligned mono
bytes. These are conservative allocation bounds, not a claim of guaranteed
free GDI memory. A color DDB's exact allocation is driver-dependent. FALSE
means no usable canvas; retry Destroy if resource handles remain after failed
cleanup. Create refuses any live handles, so it cannot silently leak/rebind.

mono=TRUE uses CreateBitmap(...,1,1,...); FALSE uses CreateCompatibleBitmap on
the original reference DC, never the initially monochrome memory DC. All new
pixels are cleared black. Monochrome is ONLY for content known to need two
colors; it is not a substitute for arbitrary color/gray/palette rendering.
Cube's Flat path stays color; its Dither/Wire paths motivated this option.

Borrow state.dc to draw using GDI. Restore caller-selected pens/brushes. Keep
the owned bitmap selected; retain MM_TEXT, zero origins, default black text /
white background and no clip region on the canvas DC. Do not select its bitmap
in another DC. Keep coordinates/dither origin fixed; only the transferred
rectangle shrinks. The helper never intercepts drawing to discover damage.

Present accepts a local rectangle (NULL = full canvas), clips it to the
surface and copies to destination origin + local coordinates using SRCCOPY.
The caller owns the destination DC and must use MM_TEXT with coordinates
already appropriate to that DC. GDI retains its actual destination clip.
Empty/clipped-away input is a successful no-op. Overflowing signed 16-bit
destination bounds fail before GDI. Mono copies explicitly map zero bits to
zeroColor and one bits to oneColor; both destination colors are restored on
success and attempted on failure. Color canvases leave destination colors
alone. Restoration failure returns FALSE, so callers must not commit damage.

Destroy restores the original bitmap, deletes the DC, then the owned bitmap.
It never deletes stock objects. It is safe to repeat, including after a partial
Create. Failed deletion retains the handle for retry and makes the state
unpresentable immediately. After Destroy begins, only retry Destroy until it
succeeds; never copy a live state or abandon retained handles.

WINDOW LIFETIME / EXPOSE
A BitBlt success is not proof that every requested destination pixel was
visible: GDI may clip it. The window must still handle WM_PAINT. Re-present
ps.rcPaint from a valid initialized surface, even when outside animation
damage. Only Commit after all animation damage is covered by the paint update
region; do not replace BeginPaint clipping. The example unions pending damage
into the update region BEFORE BeginPaint, then copies the paint bounds.

On resize, recreate the surface and Init damage to its new dimensions. On
origin/viewport changes, invalidate/erase the OLD destination area explicitly,
then Reset and invalidate the NEW area; this module does not own windows or
track screen positions. Ordinary exposes do not destroy a retained surface.
If its contents are lost/unknown, Reset and repaint the complete surface.
Minimize/pause animation at the application level; destroy on window teardown.
No direct screen painting, full-screen shadow, VRAM writes or driver escape is
needed. Never defer a display-driver operation through this application API.

EXAMPLE / CHECKS
examples/CANVAS30 is a separate moving-marker consumer: small mono surface,
coalesced timers, expose recovery, tiny-client resize/recreate and teardown.
It is source/build input, not a qualified native runtime distribution.
  python libtandywin/CANVAS/TESTS/RUN.PY --out <new-output>
Host mocks exercise actual source; they do NOT qualify Windows behavior,
native ABI, pixel equivalence, 8088 performance or GDI memory availability.

Native adoption gate: build with owned MSC6 /G0 /Gw /AS and Win3 SDK; compare
full-screen deterministic captures and GDI failures across expose, overlap,
move, resize, minimize/restore and repeated create/destroy; record CODE/DGROUP,
warm free memory, copied area and separated draw/copy times. Use the same
actual driver bytes in A/B. Cube's 160 fixture used old TNDY160, not current
TR51616, so its timing must not be generalized to the current driver/library.

PARTIAL NATIVE RESULTS (2026-10-30)
NATIVE.JSN records matched current TR51616/TR53216 tests: the unchanged helper
passed native ABI/FAR state, pixel/lifecycle, mono/color mapping, failure retry
and bounded-copy timing checks. The example additionally executes its actual
overlapped WinMain as a separate task with real timers and system window
controls, then unloads after both system Close and headless keyboard Alt-F4.
Its compact initial frame now fits the physical screen; helper C/H are
unchanged. The old TNDY160-only observation above remains historical and is
not used as current-driver evidence. These results do not qualify every
client, arbitrary driver, full-period pixel equality or physical hardware.

The sample's current failure paths are qualified separately in FAILURES.JSN.
It propagates background FillRect failure into full-client paint retry and
keeps its existing task timer/cleanup state alive after HWND destruction until
Destroy succeeds. The sample, not this helper, owns that timer and message
loop. Library C/H and the retry/ownership contract are unchanged.
