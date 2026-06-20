# Drill Calibration Data

These CSV files are protected program data. Do not edit or regenerate them without explicit project-owner approval.

- `drill_base.csv`: Per-material baseline bit size, RPM, average density, horizontal baseline, and vertical baseline.
- `exponents.csv`: Per-material scaling exponents used by the RPM and horizontal-speed formulas.
- `avg_densities.csv`: Reference wood-density table. It is not read by the current executable.
- `plunge_level.csv`: Reference plunge-level table. It is not read by the current executable.

The active calculation path currently reads only `drill_base.csv` and `exponents.csv`.
