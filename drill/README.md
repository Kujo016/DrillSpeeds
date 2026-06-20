# Drill Subengine

`drill/` is the standalone native CNC drill-speed subengine for VectorisoSimulator. It is separate from the lure simulator under `system/`.

## Current Responsibilities

- Accept a calculated or manually entered sample density.
- Select Wood, Polycarbonate, Aluminum, or Brass.
- Select or create a persistent bit profile under `drill/bits/`.
- Accept a plunge level from 1 through 10.
- Calculate spindle RPM, chip load in mm/tooth, horizontal feed speed, vertical plunge speed, and steps per drill-bit diameter.
- Write a uniquely named text report under `log/`.

## Directory Map

- `src/`: C++ entrypoint and calculation implementation.
- `include/`: `DensityChoice`, the calculation entrypoint declaration, shared includes, and the density helper.
- `data/`: Protected calibration/reference CSV files.
- `bits/`: Persistent drill-bit profiles.
- `bin/`: Generated executable and debug symbols.
- `obj/`: Generated compiler object files.
- `log/`: Generated drill-speed text reports.

Each subdirectory contains a README describing its local contract.

## Build

From the project root:

```powershell
.\win\build_drill.bat
```

The current output is `drill/bin/Drill.exe`.

## Current Run Contract

The drill executable expects argument `1` and then uses an interactive console workflow:

```powershell
.\drill\bin\Drill.exe 1
```

## Known Integration Gaps

- The current executable uses console prompts, a repeat/quit loop, and `pause`.
- `avg_densities.csv` is a reference table but is not read by the current calculation path.

## Planned PyQt6 DLL Integration

The drill calculator will eventually be exposed through a drill-specific DLL loaded by `system/python/pyqt6.py`. The future interface should:

- Accept structured inputs instead of reading from `std::cin`.
- Return structured results and explicit error/status information.
- Resolve `drill/data` independently of the process working directory.
- Avoid `pause`, quit loops, and other console ownership.
- Keep report writing optional so the GUI can display results without forcing file output.

No drill DLL or Python binding exists yet.
