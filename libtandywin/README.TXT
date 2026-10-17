libwintandy - Win16 UI helper component

The public product name is libwintandy. The historical source directory
libtandywin and API names LTWUI / LtwList... remain for compatibility.
LWTANDY.H is the small public umbrella header. Include the Windows 3.0
WINDOWS.H first; it forwards LTWUI.H and introduces no Windows declarations.
LTWUI.C and LTWUI.H are the implementation and original API header.

LtwSetText(control, text) queries USER each time. A null control returns FALSE
without a USER call. For current text lengths below 144 characters it reads
and compares the text and returns FALSE without SetWindowText when unchanged.
A changed short string is sent once and returns TRUE. Lengths of 144 or more
skip the comparison read and pass the requested text directly to SetWindowText
once. This uses a 144-byte automatic buffer and does not retain text or pixels.
Windows 3.0 SetWindowText returns void, so TRUE means a call was sent, not that
USER reported success. Do not use this helper with arbitrary owner-drawn or
side-effectful controls; it is intended for ordinary controls owned by caller.

Zero-initialize LTWLISTPAINT and pair each LtwListBegin with LtwListEnd on all
paths inside one synchronous mutation scope. Keep one list handle throughout
a nested batch, do not overflow depth, and do not yield while redraw is held.
The caller owns the window lifetime and passes its current list handle to End.
Only the outer Begin disables redraw and only the matching outer End reenables
it. The outer End sends one InvalidateRect with erase enabled only when the
saved handle equals the supplied current handle and IsWindow still recognizes
it. A destroyed or replaced handle is not re-enabled or invalidated; an extra
End at depth zero does nothing. The caller must still coordinate destruction
and replacement with the batch lifetime.

A completed batch requests a normal USER/GDI repaint including background
erasure. It does not preserve pixels or promise visual equivalence with any
other renderer. This component does not add widget behavior, hooks, timers,
display-driver access, or framebuffer writes. It does not change DOS/hardware
libtandy.

TESTS/RUN.PY builds the real LTWUI.C against a minimal original host USER stub
and exercises text and batch calls. These mock checks cover call behavior only;
they do not establish Windows visual behavior, native compatibility, or
hardware equivalence.
