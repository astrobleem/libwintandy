# Current format tests

HOST/RUNHOST.PY runs the format, actual RAWCLK fake-hardware and MOD parser
suites against SOURCE. INPUTS holds original synthetic fixtures; MAKE_INPUTS.PY
regenerates them. Paths are relative to the AUDIO root. The source assumes the
MSC6/Windows unsigned-long32 ABI; LP64 is not currently supported unchanged.

The saved native/build logs and source-inclusive tested runtime are in the
FORMATS30 preview archive linked from examples/FORMATS30/README.md. QUALIFY.MD
records 171 native checks with zero failures and explicit result boundaries.
The old HOST/PLAIN.JSON and FACADE records remain historical core evidence.
