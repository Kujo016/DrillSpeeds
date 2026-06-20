# Drill Headers

- `cnc.h`: Defines `DensityChoice` and declares `run_drill_speeds()`.
- `tools.h`: Defines the inline rectangular-volume density helper.
- `globals.h`: Shared standard-library includes used by the drill sources.

The future DLL interface should use an explicit public header with stable input, result, status, and error-buffer contracts rather than exposing the interactive function directly.
