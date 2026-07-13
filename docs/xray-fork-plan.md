# Nekoray Xray-core fork: stage 1 repository map

This document records the first, non-functional preparation pass for a future Nekoray fork based on a modern Xray-core. No UI rewrite or protocol implementation is included in this stage.

## Reproducible Debug build

Use the checked-in full Linux Debug preset after bootstrapping Qt, submodules, and C/C++ dependencies:

```bash
scripts/bootstrap-linux.sh --fetch-core-sources --build-cpp-deps
cmake --preset linux-qt5-debug
cmake --build --preset linux-qt5-debug
```

A reduced analysis preset is also available for environments without optional C++ dependencies:

```bash
cmake --preset debug-no-external
cmake --build --preset debug-no-external
```

The confirmed Linux release path in GitHub Actions builds inside `ghcr.io/matsuridayo/debian10-qt5:20230131` with Qt 5. Windows CI uses Qt 6.7, so both Qt major versions are supported by the CMake project, but Linux is documented and automated as Qt 5.

## Toolchain and dependency inventory

Observed in the current container:

| Component | Observed / declared version | Source |
| --- | --- | --- |
| CMake runtime | 3.28.3 | `cmake --version` |
| Project minimum CMake | 3.5 | `CMakeLists.txt` |
| C++ standard | C++17 | `CMakeLists.txt` |
| GCC / g++ | 13.3.0 | `gcc --version`, `c++ --version` |
| Clang | 17.0.0 | `clang++ --version` |
| Go runtime | 1.25.1 observed locally; GitHub Actions installs `^1.22` | `go version`, `.github/workflows/build-nekoray-cmake.yml` |
| Qt | Qt 5 by default; Linux CI uses Qt 5.12 container; Windows CI uses Qt 6.7 | `CMakeLists.txt`, `.github/workflows/build-nekoray-cmake.yml` |
| Required Qt modules | Widgets, Network, Svg, LinguistTools | `CMakeLists.txt` |
| Optional C++ dependencies | protobuf/gRPC generated C++ proto, yaml-cpp, zxing-cpp, QHotkey | `CMakeLists.txt`, `cmake/myproto.cmake` |
| Vendored/embedded C++ utilities | Qv2ray UI/config helpers, GeositeReader, qrcodegen, base64, qscopeguard, VT100 parser | `3rdparty/` |
| Go gRPC server module | `go 1.19`; grpc `v1.49.0`; protobuf `v1.28.1` | `go/grpc_server/go.mod` |
| Current core module | `go 1.19`; sing-box `v1.0.0` replaced to local `../../../../sing-box`; libneko replaced to local `../../../../libneko` | `go/cmd/nekobox_core/go.mod` |
| Updater module | `go 1.18` | `go/cmd/updater/go.mod` |
| Dependency build scripts | zxing-cpp `v2.0.0`, yaml-cpp `0.7.0`, protobuf `v21.4`; pinned Go core sources from `libs/get_source_env.sh` | `libs/build_deps_all.sh`, `libs/get_source.sh`, `libs/get_source_env.sh` |

## Current proxy core architecture

### GUI process supervision

- `MainWindow` starts a companion executable named `nekobox_core` from the application directory and passes `nekobox -port <port>` plus `-debug` when enabled.
- `NekoGui_sys::CoreProcess` extends `ExternalProcess`, writes the generated core token to stdin, watches stdout for `grpc server listening`, and marks the core as running only after that signal.
- If the core exits unexpectedly, `CoreProcess` rate-limits automatic restarts and asks the UI layer to restart the active profile after the gRPC server comes back.
- `NekoGui_sys::ExternalProcess` is the generic abstraction for auxiliary external cores; it stores program, arguments, environment, logging, crash state, and kill semantics.

### gRPC control plane

- The GUI talks to the local core through `NekoGui_rpc::Client` and generated `libcore` protobuf messages.
- `MainWindow::neko_start()` builds the selected profile into a core JSON config, sends `LoadConfigReq` to `Client::Start()`, enables traffic/stat loops, then starts any mapped external processes.
- `MainWindow::neko_stop()` stops profiles through the same gRPC client and cleans up external processes.

### Config builder and profile model

- `BuildConfig()` is the central config assembly path. It creates sing-box-style inbounds, outbounds, DNS, route, experimental Clash API settings, and TUN/VPN config.
- Protocol beans in `fmt/` convert UI/profile data to links, external commands, and sing-box core JSON. The primary modern internal-core path is `BuildCoreObjSingBox()`.
- Custom profiles can target `internal`, `internal-full`, or a named external core from `ExtraCore::core_map`.
- `ExtraCore` stores user-defined external core mappings as JSON id-to-path entries; there is intentionally no default external core mapping.

## Existing Xray/v2ray traces

- The README credits historical v2fly/v2ray, Matsuri v2ray, XTLS/Xray-core, and MatsuriDayo/Xray-core backends for older version ranges.
- `3rdparty/qv2ray/` remains in the tree and is still used for JSON editor, proxy configurator, autocomplete, and geosite reader UI helpers.
- Link parsing and export code still references v2rayN formats and Xray documentation for some VMess/VLESS/Trojan URL conventions.
- Route and config comments retain v2ray terminology, for example outbound tags and domain matcher mappings, while the active generated core config is sing-box-style.
- Packaging scripts and README still contain some legacy wording such as `backend: v2ray / sing-box`, but runtime startup currently targets `nekobox_core`.

## Build and test findings

### C++/Qt build

Command attempted:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DNKR_NO_EXTERNAL=ON -DQT_VERSION_MAJOR=5
cmake --build build/debug -j$(nproc)
```

Result in this container: configuration fails before compilation because Qt development CMake package files are not installed:

```text
Could not find a package configuration file provided by "Qt5" with any of the following names:
  Qt5Config.cmake
  qt5-config.cmake
```

This is an environment/dependency issue, not a source change. Install Qt 5 development packages, or point `Qt5_DIR`/`CMAKE_PREFIX_PATH` to a Qt SDK, then rerun the preset build.

### Dependency bootstrap and Go tests

Commands attempted:

```bash
(cd go/grpc_server && go test ./...)
(cd go/cmd/updater && go test ./...)
(cd go/cmd/nekobox_core && go test ./...)
git submodule update --init --recursive
bash libs/get_source.sh
bash libs/build_deps_all.sh
```

Results:

- `git submodule update --init --recursive`: failed to clone `https://github.com/Skycoder42/QHotkey.git` because the environment returned `CONNECT tunnel failed, response 403`.
- `bash libs/get_source.sh`: failed to clone `https://github.com/MatsuriDayo/sing-box.git` with the same `CONNECT tunnel failed, response 403`; this script is the official source of the sibling `libneko`, `sing-box`, and `sing-quic` directories.
- `bash libs/build_deps_all.sh`: failed on the first download, `https://github.com/nu-book/zxing-cpp/archive/refs/tags/v2.0.0.zip`, with `curl: (56) CONNECT tunnel failed, response 403`.
- `go/grpc_server`: setup failed because the Go proxy returned `Forbidden` for grpc/protobuf modules and local replacement `../../../libneko` is absent.
- `go/cmd/updater`: setup failed because the Go proxy returned `Forbidden` for `github.com/codeclysm/extract`.
- `go/cmd/nekobox_core`: setup failed because local replacements `../../../../libneko`, `../../../../sing-box`, and `../../../../sing-quic` are absent; grpc/protobuf downloads were also blocked.

## Component map for the Xray-core fork

| Area | Current files | Role for future Xray work |
| --- | --- | --- |
| Core process lifecycle | `sys/ExternalProcess.*`, `ui/mainwindow.cpp`, `ui/mainwindow_grpc.cpp` | Add an Xray-capable core process strategy without breaking the existing `nekobox_core` lifecycle. |
| Core RPC API | `rpc/gRPC.*`, `go/grpc_server/gen/libcore.proto`, `go/grpc_server/` | Decide whether Xray is controlled through the existing gRPC wrapper, a new wrapper, or direct process/config file execution. |
| Config assembly | `db/ConfigBuilder.*`, `fmt/Bean2CoreObj_box.cpp`, `fmt/Bean2External.cpp` | Introduce an Xray config builder parallel to the sing-box builder; avoid mutating UI beans first. |
| Protocol beans | `fmt/*Bean*`, `ui/edit/*` | Reuse existing profile fields for VLESS, VMess, Trojan, Shadowsocks, WireGuard, Hysteria2, transports, TLS/REALITY/XTLS where possible. |
| Subscription/import/export | `sub/`, `fmt/Link2Bean.cpp`, `fmt/Bean2Link.cpp` | Extend parsing/export compatibility for 3x-ui subscriptions after the Xray config path exists. |
| Traffic/statistics | `db/traffic/TrafficLooper.*`, `rpc/gRPC.*`, `go/grpc_server/` | Map Xray stats API counters to existing total/used/remaining display model; subscription quota metadata likely belongs in group/profile metadata. |
| TUN/VPN | `ui/dialog_vpn_settings.*`, `db/ConfigBuilder.cpp`, `res/vpn/sing-box-vpn.json` | Keep current sing-box TUN path initially; evaluate Xray + external TUN only after core abstraction is stable. |
| External-core mappings | `main/NekoGui.cpp` (`ExtraCore`), `fmt/Bean2External.cpp`, `ui/edit/edit_custom.*` | Useful for transitional Xray experiments before internalizing Xray support. |

## Recommended next stages

1. Install/build missing dependencies and get the existing Debug preset compiling without source changes.
2. Restore local Go replacement repositories (`libneko`, `sing-box`, `sing-quic`) or replace them with reproducible module sources.
3. Add an internal `CoreKind`/`CoreBackend` abstraction around config generation and process/RPC lifecycle.
4. Add an Xray JSON builder in parallel with the current sing-box builder.
5. Add 3x-ui subscription compatibility tests using saved fixtures before changing import code.
6. Add traffic quota/expiry metadata to group/profile models separately from 3x-ui admin API support.
