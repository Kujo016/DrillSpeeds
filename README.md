# DrillSpeeds

DrillSpeeds is a standalone native CNC drill-speed calculation engine.

## Features and Functionality (`drill/`)
The `drill/` subdirectory contains the core logic for the drill speed calculations. Its functionality includes:
- **Density Input**: Accepts a calculated or manually entered sample density.
- **Material Selection**: Supports Wood, Polycarbonate, Aluminum, and Brass.
- **Bit Profiles**: Selects or creates persistent drill bit profiles (stored in `drill/bits/`).
- **Plunge Levels**: Accepts a plunge level from 1 through 10.
- **Calculations**: Computes spindle RPM, chip load in mm/tooth, horizontal feed speed, vertical plunge speed, and steps per drill-bit diameter.
- **Reporting**: Generates uniquely named text reports saved in the `drill/log/` directory.

## How to Build
This project uses the [`minimum_project`](https://github.com/Kujo016/minimum_project) repository as its build system. This is a standalone repo, but it references scripts and tools from the `minimum_project` to build the engine.

To build the project:
1. Ensure you have the `win/` and `builder/` scripts from `minimum_project` available https://github.com/Kujo016/minimum_project repository.
2. Run the build script located in the `win/` directory:
   ```powershell
   .\win\build_drill.bat
   ```

## How to Run the CLI
The DrillSpeeds tool provides an interactive console workflow. You can start the CLI using the provided batch script in the root directory:

```powershell
.\run_drill_speeds.bat
```

This script will run the built executable (`drill\bin\Drill.exe 1`) and guide you through an interactive console session to configure your materials, bits, and calculate the appropriate drill speeds.

## Requirements
Please see [REQUIREMENTS.md](REQUIREMENTS.md) for detailed environment and build requirements.
