# Dynamic distance-header compatibility

GUNZIP now accepts 31 or 32 declared distance-code lengths. RFC1951 section
3.2.7 permits that header range; reserved distance symbols 30 and 31 remain
invalid if actually decoded. The production change is one ASCII byte:
`ndist>30` becomes `ndist>32`. The existing `sym<0 || sym>=30` rejection still
precedes every access to the 30-entry distance base/extra tables.

Capacity audit: maximum literal/length count 286 plus maximum distance count
32 is 318, within `lens[320]`. Each Huffman tree has 288 symbol slots; the
distance tree needs at most 32. No buffer, stack, data structure, playback
format, timer, or ownership change is required. Other bounded-profile refusals
are unchanged. Reference: https://www.rfc-editor.org/rfc/rfc1951#section-3.2.7

HOST/MAKE_GZVECT.PY generates original synthetic cases. They include the exact
reviewer `A` reproducer, 31/32-entry headers at maximum HLIT, valid distance-1
copies, and actual reserved-symbol use in both dynamic and fixed trees. The
negative cases emit their leading literal before returning Huffman error 3,
so the checks establish that header construction was passed. An output canary
also remains unchanged. The generated GZ31/GZ32 files contain the same original
SHORT.VGM bytes; RS30/RS31 deliberately contain reserved distance use.

Recorded host results: the original header bound fails 22 assertions in the
expanded 160-check format suite. The one-byte correction passes all 160,
alongside 3,271 backend checks and the MOD suite. These are author-run tests,
not public CI or an independent reviewer rerun. The targeted 16-bit rebuild and 88-check native decoder/MINIPLAY UI case pass.
See TESTS/GZIP06 for their exact current source/binary identities.

The current 47,997-byte MINIPLAY differs from BUILD04 in one comparison-limit byte (30 to 32) and its NE checksum field. Data/stack/file sizes do not grow. No calibrated CPU measurement is claimed.
