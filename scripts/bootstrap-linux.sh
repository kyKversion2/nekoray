#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: scripts/bootstrap-linux.sh [--check] [--fetch-core-sources] [--build-cpp-deps] [--download-public-res] [--install-system-packages]

Prepares the Linux build workspace without changing system packages by default.

Options:
  --check                    Only verify commands, submodule/core-source state, and Qt availability.
  --fetch-core-sources       Run libs/get_source.sh to clone pinned libneko, sing-box, and sing-quic next to this repo.
  --build-cpp-deps           Run libs/build_deps_all.sh to build pinned C++ dependencies under libs/deps.
  --download-public-res      Run libs/build_public_res.sh to download geodata/public resources.
  --install-system-packages  Install documented Debian/Ubuntu packages with apt-get. Requires sudo/root.
  -h, --help                 Show this help.
USAGE
}

log() { printf '[bootstrap] %s\n' "$*"; }
fail() { printf '[bootstrap] ERROR: %s\n' "$*" >&2; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

CHECK_ONLY=0
FETCH_CORE_SOURCES=0
BUILD_CPP_DEPS=0
DOWNLOAD_PUBLIC_RES=0
INSTALL_SYSTEM_PACKAGES=0

while (($#)); do
  case "$1" in
    --check) CHECK_ONLY=1 ;;
    --fetch-core-sources) FETCH_CORE_SOURCES=1 ;;
    --build-cpp-deps) BUILD_CPP_DEPS=1 ;;
    --download-public-res) DOWNLOAD_PUBLIC_RES=1 ;;
    --install-system-packages) INSTALL_SYSTEM_PACKAGES=1 ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; fail "Unknown option: $1" ;;
  esac
  shift
done

REPO_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$REPO_ROOT"

REQUIRED_COMMANDS=(git cmake ninja curl unzip go)
OPTIONAL_COMMANDS=(qmake qmake6 pkg-config)

log "Repository: $REPO_ROOT"
log "Checking required commands"
missing=()
for cmd in "${REQUIRED_COMMANDS[@]}"; do
  if have "$cmd"; then
    log "found $cmd: $(command -v "$cmd")"
  else
    missing+=("$cmd")
  fi
done
if ((${#missing[@]})); then
  fail "Missing required commands: ${missing[*]}. Install the packages listed in docs/build-linux.md."
fi

log "Checking optional Qt discovery commands"
for cmd in "${OPTIONAL_COMMANDS[@]}"; do
  if have "$cmd"; then
    log "found $cmd: $(command -v "$cmd")"
  else
    log "$cmd not found; CMake can still work if Qt is provided through CMAKE_PREFIX_PATH or Qt5_DIR/Qt6_DIR"
  fi
done

if ((INSTALL_SYSTEM_PACKAGES)); then
  if ! have apt-get; then
    fail "--install-system-packages currently supports Debian/Ubuntu systems with apt-get only."
  fi
  log "Installing Debian/Ubuntu build packages with apt-get"
  sudo apt-get update
  sudo apt-get install -y \
    build-essential git cmake ninja-build curl unzip pkg-config golang-go \
    qtbase5-dev qttools5-dev qttools5-dev-tools libqt5svg5-dev libqt5x11extras5-dev
fi

log "Initializing Git submodules"
if ((CHECK_ONLY)); then
  git submodule status --recursive || fail "Unable to inspect submodule status."
  if git submodule status --recursive | grep -q '^-'; then
    fail "Some submodules are not initialized. Run scripts/bootstrap-linux.sh without --check, or run git submodule update --init --recursive."
  fi
else
  git submodule update --init --recursive || fail "Failed to initialize submodules. Check network access to URLs in .gitmodules."
fi

log "Checking Qt CMake package availability"
qt_found=0
if cmake -S . -B build/bootstrap-qt-check -GNinja -DQT_VERSION_MAJOR=5 -DNKR_NO_EXTERNAL=ON >/tmp/nekoray-bootstrap-qt5.log 2>&1; then
  qt_found=1
  log "Qt5 CMake package found"
else
  log "Qt5 check failed; see /tmp/nekoray-bootstrap-qt5.log"
fi
if ((qt_found == 0)); then
  if cmake -S . -B build/bootstrap-qt6-check -GNinja -DQT_VERSION_MAJOR=6 -DNKR_NO_EXTERNAL=ON >/tmp/nekoray-bootstrap-qt6.log 2>&1; then
    qt_found=1
    log "Qt6 CMake package found"
  else
    log "Qt6 check failed; see /tmp/nekoray-bootstrap-qt6.log"
  fi
fi
if ((qt_found == 0)); then
  fail "Neither Qt5 nor Qt6 CMake packages were found. Install Qt development packages or set CMAKE_PREFIX_PATH/Qt5_DIR/Qt6_DIR."
fi

if ((CHECK_ONLY)); then
  log "Check completed successfully. No downloads or builds were performed."
  exit 0
fi

if ((FETCH_CORE_SOURCES)); then
  log "Fetching pinned Go core source repositories with libs/get_source.sh"
  bash libs/get_source.sh || fail "Failed to fetch pinned core sources. Check network access to GitHub and libs/get_source_env.sh commits."
else
  log "Skipping core source fetch. Use --fetch-core-sources to clone libneko, sing-box, and sing-quic."
fi

if ((BUILD_CPP_DEPS)); then
  log "Building C++ dependencies with libs/build_deps_all.sh"
  bash libs/build_deps_all.sh || fail "Failed to build C++ dependencies. Check network access and toolchain packages."
else
  log "Skipping C++ dependency build. Use --build-cpp-deps to populate libs/deps."
fi

if ((DOWNLOAD_PUBLIC_RES)); then
  log "Downloading public resources with libs/build_public_res.sh"
  bash libs/build_public_res.sh || fail "Failed to download public resources. Check network access to geodata release URLs."
else
  log "Skipping public resource downloads. Use --download-public-res when packaging."
fi

log "Bootstrap completed. Configure with: cmake --preset linux-qt5-debug"
