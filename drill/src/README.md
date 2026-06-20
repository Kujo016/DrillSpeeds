# Drill Source

## Files

- `main.cpp`: Interactive executable entrypoint. Handles density input, calls `run_drill_speeds()`, repeats until the user quits, and currently ends with `pause`.
- `drill_speeds.cpp`: Loads material calibration rows, calculates drill settings, prints results, and writes a unique text report.

## Call Path

`main(argc, argv)` -> collect `DensityChoice` -> `run_drill_speeds()` -> load calibration rows -> calculate RPM/feed/plunge/steps -> print and write report.

## Current Formulas

- Density: `mass / (length * width * height)`.
- RPM: baseline RPM scaled by bit-size and density exponent terms.
- Horizontal speed: baseline horizontal speed scaled by bit-size and density exponent terms.
- Vertical speed: horizontal speed multiplied by density and normalized plunge level.
- Steps: target drill-bit size multiplied by two.

## Planned Boundary

Future DLL work should separate the pure calculation and data-loading path from the interactive console entrypoint so `system/python/pyqt6.py` can call it safely.
