#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -x c -std=c99 -DSYNTH_HOST -I TESTS/mdhost -I SOURCE \
 TESTS/mdhost/driver.c SOURCE/MIDISYN.C SOURCE/INSTR.C -o /tmp/tandy-midi-host
/tmp/tandy-midi-host
cc -x c -std=c99 -DSYNTH_HOST -I SOURCE \
 TESTS/synth_host.c SOURCE/MIDISYN.C SOURCE/INSTR.C -o /tmp/tandy-synth-host
/tmp/tandy-synth-host
cc -x c -std=gnu99 -DSYNTH_HOST -I TESTS/mdhost -I SOURCE \
 TESTS/mdctl_host.c SOURCE/MIDISYN.C SOURCE/INSTR.C -o /tmp/tandy-mdctl-host
/tmp/tandy-mdctl-host
cc -x c -std=gnu99 -DSMF_FAR= -I SOURCE TESTS/smf_stream_host.c SOURCE/SMF.C -o /tmp/tandy-smf-stream
/tmp/tandy-smf-stream TESTS/FIXTURES/*.MID
