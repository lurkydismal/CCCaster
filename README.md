# CCCaster

> Experimental modding toolkit and Luau modloader for Melty Blood Actress Again Current Code (MBAACC).

## Table of Contents

* [General Information](#general-information)
* [Technologies Used](#technologies-used)
* [Features](#features)
* [Architecture](#architecture)
* [Setup](#setup)
* [Usage](#usage)
* [Addon Structure](#addon-structure)
* [Project Status](#project-status)
* [Room for Improvement](#room-for-improvement)
* [Acknowledgements](#acknowledgements)
* [License](#license)

## General Information

CCCaster is a work-in-progress modding framework for the 32-bit `MBAA.exe` game process. It combines a command-line launcher, a native wrapper, and a Rust-based modloader to load Luau addons, apply binary patches, and expose runtime APIs to scripts.

The repository uses the name CCCaster, but this project focuses on mod loading and runtime tooling rather than reproducing the original CCCaster netplay client.

## Technologies Used

* Rust 2024 edition and Cargo.
* GNU C++26 for the native wrapper.
* Luau, embedded through `mlua`, for addon scripting.
* Wasmtime for WebAssembly-based asset conversion support.
* Win32 APIs and Wine/Winelib for the 32-bit process integration path.
* Docker, `just`, `ccache`, and `mold` for the optional containerized wrapper build.

## Features

* Addon discovery from an `addons/` directory using `info.json` metadata.
* Semantic-version dependency checks, dependency-graph load ordering, and circular-dependency detection.
* Binary patch definitions in `patch.jsonc`, including checks for overlapping patch ranges.
* Luau addon lifecycle callbacks, event dispatch, per-addon script loading, and a custom `require` API.
* Runtime APIs for logging, process-memory reads and writes, byte patches, and detour hooks.
* Filesystem and asset overlay support, including directory-based assets and compressed `assets.tar.zstd` archives.
* File-watcher-based hot reload for addon scripts, patches, and assets.
* Launcher options for addon selection, profiles, game-argument forwarding, process attachment, and diagnostic logging.
* Diagnostic options for printing resolved patches and dependency graphs, measuring startup phases, and enabling more verbose logs.

## Architecture

* **`launcher/`** — Rust command-line launcher. Resolves runtime paths and profiles, starts `MBAA.exe`, and injects the wrapper into the game process. The current native Linux launcher implementation is a stub; the implemented launcher path is Windows-oriented.
* **`wrapper/`** — 32-bit C++/Winelib wrapper. Provides the native API for memory access, patches, and detours, then loads the modloader library.
* **`cccaster/`** — Rust shared library containing addon discovery, dependency resolution, the Luau runtime, patch/hook coordination, hot reload, and asset overlays.
* **`addons/core/`** — Example core addon and the default set of game-specific patches.

## Setup

### Prerequisites

* A compatible 32-bit MBAACC installation containing `MBAA.exe`.
* A Rust toolchain and the Rust targets `i686-unknown-linux-gnu` and `i686-pc-windows-gnu`.
* A Wine/Winelib build environment with `wineg++`, plus the 32-bit build dependencies required by the wrapper.
* For the containerized wrapper build: Docker and [`just`](https://github.com/casey/just).

The wrapper build script uses `wineg++` and produces `wrapper.dll`; it is not a conventional native Windows C++ build. The runtime is still under development, so a successful compile does not guarantee compatibility with every game build.

### Build

Run the build scripts from the repository root:

```bash
# Build the modloader shared library for the 32-bit Linux/Wine side.
(cd cccaster && ./build.sh -l -r)

# Build the 32-bit Windows launcher.
(cd launcher && ./build.sh -w -r)

# Build the 32-bit Winelib wrapper (requires wineg++ and its toolchain).
(cd wrapper && ./build.sh -l -r)
```

Alternatively, build the wrapper in the provided Docker environment:

```bash
cd wrapper
just docker-build-image
just build-release
```

### Assemble the runtime

Place the built files and the repository's `addons/` directory in the directory from which you will run the launcher. That directory must also contain `MBAA.exe`.

For example, set `GAME_DIR` to your game directory and copy the artifacts there:

```bash
GAME_DIR=/path/to/your/MBAACC

cp launcher/target/i686-pc-windows-gnu/release/launcher.exe "$GAME_DIR/"
cp wrapper/wrapper.dll "$GAME_DIR/"
cp cccaster/target/i686-unknown-linux-gnu/release/libcccaster.so "$GAME_DIR/main.so"
cp -r addons/core "$GAME_DIR/addons/"
```

The wrapper currently looks for the modloader library at `./main.so`, so the library must be named `main.so` and available in the launcher's working directory. Merge `addons/core/` into an existing addons directory rather than replacing other addons you keep there.

## Usage

Launch from the game directory so the default paths resolve correctly:

```bash
cd "$GAME_DIR"
wine ./launcher.exe
```

The launcher supports command-line options for selecting or disabling addons, choosing a profile, attaching to a running game process, passing arguments through to the game, and enabling diagnostics. Examples:

```bash
# Show all supported options.
wine ./launcher.exe --help

# Launch with additional game arguments.
wine ./launcher.exe -- -fullscreen

# Increase logging verbosity and print patch/dependency diagnostics.
wine ./launcher.exe -vv --dump-graph --dump-patches

# Load a named profile from profiles/<name>.json.
wine ./launcher.exe --profile modded
```

Profiles are JSON files stored under `profiles/`. They can hold enabled and disabled addons, an explicit load order, environment overrides, and game arguments. CLI options and some runtime behavior are still evolving; consult `--help` for the options available in your build.

## Addon Structure

A minimal addon has an `info.json` metadata file and a `main.luau` entry point. It may also include a `patch.jsonc` file for binary patches and an `assets/` directory or `assets.tar.zstd` archive for assets.

```text
addons/
└── example/
    ├── info.json
    ├── main.luau
    ├── patch.jsonc       # optional
    └── assets/           # optional
```

`info.json` describes the addon version, dependencies, supported API version, and lifecycle events. `main.luau` can return callbacks such as `init`, `post_init`, `load`, `unload`, and `quit`. The `addons/core/` directory provides a starting point, but its hook example is diagnostic scaffolding rather than a finished production addon.

## Project Status

Project is: _in progress_. The launcher, wrapper, and modloader are developed as separate components, and the end-to-end runtime still requires testing against the intended game environment. The native Linux launcher backend is currently unsupported.

## Room for Improvement

To do:

* Complete and test the launcher/runtime path and addon-directory configuration.
* Finish the native Linux launcher backend, if native Linux launching remains a goal.
* Turn the core addon into a complete, documented example and improve hook-error handling.
* Add automated integration tests for injection, unload/shutdown, patch restoration, hot reload, and asset overlays.
* Document supported game executable versions and validate game-specific patch addresses against them.

## Acknowledgements

* [Luau](https://github.com/luau-lang/luau) provides the addon scripting language.
* [Wasmtime](https://wasmtime.dev/) provides WebAssembly runtime support for asset conversion.
* The project also relies on the Rust ecosystem and its open-source dependencies.

## License

This project is open source and available under the [GNU Affero General Public License v3.0](LICENSE).
