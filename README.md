# SwarmRogue

An Unreal Engine 5.4 C++ project for a networked swarm-survival game.

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.4-0E1128?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![C++](https://img.shields.io/badge/C%2B%2B-Game%20Code-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview

SwarmRogue is a C++ gameplay project built around Unreal Engine's gameplay framework. It combines Gameplay Ability System (GAS), replicated player state, server-authoritative gameplay, and a pooled spatial-hash swarm subsystem for large enemy populations.

The repository intentionally excludes Unreal asset files and generated build data. This keeps the source repository small and makes the boundary between code and separately managed game content explicit.

## Documentation

The detailed technology inventory, implementation map, networking model, performance notes, and scanned study references are maintained in [`Docs/technology-stack.md`](Docs/technology-stack.md).

## Repository Layout

```text
Config/                            Project and gameplay configuration
Docs/                              Architecture, technology charts, and references
Plugins/                           Plugin source and descriptors
Source/SwarmRogue/                 Runtime C++ module
Source/SwarmRogueEditor.Target.cs  Editor target definition
SwarmRogue.uproject                Unreal project descriptor
```

## Requirements

- Windows 10 or later
- Unreal Engine 5.4
- Visual Studio 2022 with **Game development with C++** installed
- Git

## Getting Started

1. Clone the repository:

   ```bash
   git clone https://github.com/huangnaisheng/SwarmRougle.git
   cd SwarmRogue
   ```

2. Install Unreal Engine 5.4 and the required Visual Studio workload.
3. Right-click `SwarmRogue.uproject` and choose **Generate Visual Studio project files**.
4. Open the generated solution in Visual Studio.
5. Select `Development Editor` and `Win64`, then build the `SwarmRogueEditor` target.
6. Open `SwarmRogue.uproject` in Unreal Editor.

## Content and Generated Files

The following are intentionally excluded from Git:

- `Content/` and Unreal asset files such as `.uasset` and `.umap`
- `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/`
- IDE metadata and packaged build output

The project cannot be reproduced as a complete playable build from this repository alone. The matching game content must be provided separately.

## Third-Party Plugins

The repository includes source code for `SPCRJointDynamics` and `SwitchLanguage`. Their upstream terms and licenses remain applicable to those components. See each plugin descriptor and source distribution for details.

## Contributing

Keep generated Unreal files and local assets out of commits. Submit focused changes with a clear commit message and verify that the project still compiles in Unreal Engine 5.4.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for the full text.
