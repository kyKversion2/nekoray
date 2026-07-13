# Linux build bootstrap and reproducible build

This page documents the confirmed upstream build path for the current Nekoray sources. It does **not** add Xray-core, new protocols, or UI changes.

## Supported Linux build environment

The official GitHub Actions Linux build uses an `ubuntu-20.04` runner only as the host for Docker and performs the actual C++ build inside:

```text
ghcr.io/matsuridayo/debian10-qt5:20230131
```

The same workflow builds the Linux GUI with Qt 5. The Windows workflow uses Qt 6.7, and the repository has a separate `test/test-qt6-build.sh`, so the CMake project supports both Qt 5 and Qt 6; however, the confirmed Linux release path is Qt 5.

## Required system packages

For a Debian/Ubuntu host or container, install the build tools and Qt 5 development packages:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential git cmake ninja-build curl unzip pkg-config golang-go \
  qtbase5-dev qttools5-dev qttools5-dev-tools libqt5svg5-dev libqt5x11extras5-dev
```

Notes:

- The upstream Linux release container provides the Qt 5 toolchain. Existing documentation allows Qt `5.12.x` or `5.15.x` for Linux builds.
- CMake must be new enough to support the checked-in presets (`3.21+` for presets v3). The project source itself declares a lower minimum, but the preset workflow needs a newer CMake.
- Ninja is the documented generator in build scripts and CI.
- Go is required for `nekobox_core` and `updater`; GitHub Actions currently installs Go `^1.22` for release builds.

## External dependency sources

### Git submodules

The only repository submodule declared in `.gitmodules` is:

```text
3rdparty/QHotkey -> https://github.com/Skycoder42/QHotkey.git
```

Initialize it with:

```bash
git submodule update --init --recursive
```

### Go core source repositories

The Go module `go/cmd/nekobox_core/go.mod` uses local `replace` directives, so the following repositories must exist next to the `nekoray` checkout, not inside it:

```text
../libneko
../sing-box
../sing-quic
```

The official script for fetching exact pinned revisions is:

```bash
bash libs/get_source.sh
```

It clones and checks out the commits declared in `libs/get_source_env.sh`:

| Repository | URL | Pinned commit |
| --- | --- | --- |
| sing-box | `https://github.com/MatsuriDayo/sing-box.git` | `06557f6cef23160668122a17a818b378b5a216b5` |
| sing-quic | `https://github.com/MatsuriDayo/sing-quic.git` | `b49ce60d9b3622d5238fee96bfd3c5f6e3915b42` |
| libneko | `https://github.com/MatsuriDayo/libneko.git` | `1c47a3af71990a7b2192e03292b4d246c308ef0b` |

Do not replace these directories with stubs. Do not copy third-party source manually into this repository.

### C/C++ dependencies

If the distribution packages do not provide compatible C++ dependencies, build the pinned dependencies with:

```bash
bash libs/build_deps_all.sh
```

That script downloads and installs under `libs/deps/built` by default:

| Dependency | Source |
| --- | --- |
| zxing-cpp `v2.0.0` | `https://github.com/nu-book/zxing-cpp/archive/refs/tags/v2.0.0.zip` |
| yaml-cpp `0.7.0` | `https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.zip` |
| protobuf `v21.4` | `https://github.com/protocolbuffers/protobuf` with tag `v21.4` |

The top-level CMake file appends `libs/deps/built` to `CMAKE_PREFIX_PATH` unless `NKR_LIBS`, `NKR_PACKAGE`, or `NKR_DISABLE_LIBS` changes that behavior.

### Public resources for packaging

Release packaging downloads geodata/public resources with:

```bash
bash libs/build_public_res.sh
```

It fetches release assets from Loyalsoldier/v2ray-rules-dat, v2fly/domain-list-community, SagerNet/sing-geoip, and SagerNet/sing-geosite.

## Bootstrap script

A non-interactive Linux helper is available:

```bash
scripts/bootstrap-linux.sh --check
scripts/bootstrap-linux.sh --fetch-core-sources --build-cpp-deps
```

`--check` verifies commands, submodule state, and Qt CMake package discovery without downloading or building dependencies. By default the script does not install system packages. Use `--install-system-packages` only when you explicitly want it to call `apt-get` on a Debian/Ubuntu machine.

## Configure and build the GUI

Full Debug build with Qt 5 and external C++ dependencies:

```bash
cmake --preset linux-qt5-debug
cmake --build --preset linux-qt5-debug
```

Reduced analysis build without optional C++ dependencies:

```bash
cmake --preset debug-no-external
cmake --build --preset debug-no-external
```

The full Debug preset expects either system packages or `libs/deps/built` to provide protobuf/gRPC, yaml-cpp, zxing-cpp, and the QHotkey submodule. The reduced preset sets `NKR_NO_EXTERNAL=ON` and is useful only for local analysis; it is not the full release-equivalent build.

## Build Go components

After `libs/get_source.sh` has populated the sibling repositories:

```bash
GOOS=linux GOARCH=amd64 bash libs/build_go.sh
```

The script builds `updater` and `nekobox_core` into `deployment/linux64` and renames the Linux updater binary to `launcher`.

## Output locations

| Component | Output |
| --- | --- |
| GUI Debug binary | `build/linux-qt5-debug/nekobox` |
| Reduced Debug binary | `build/debug-no-external/nekobox` |
| Go Linux binaries | `deployment/linux64/launcher`, `deployment/linux64/nekobox_core` |
| Packaged Linux deployment | `deployment/linux64` after `libs/deploy_linux64.sh` |

## Known limitations in restricted environments

- GitHub access is required for `3rdparty/QHotkey`, `libneko`, `sing-box`, `sing-quic`, and C/C++ dependency source downloads.
- Go module downloads may fail if `proxy.golang.org` or direct module hosts are blocked. The current repository does not vendor Go modules.
- The current container used for this stage did not include Qt5 or Qt6 development CMake package files, so CMake configure could not proceed to compilation.
- Do not disable TLS verification and do not use untrusted mirrors to bypass network policy. Treat these as environment blockers, not source-code failures.
