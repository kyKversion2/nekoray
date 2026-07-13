# Xray-core CLI contract for the isolated runner

This document records the minimal Xray-core command-line behavior used by the first isolated runner stage. It intentionally does not define any profile conversion or runtime integration.

## Verified upstream source

- Repository: <https://github.com/XTLS/Xray-core>
- Checked source commit: `50231eaff98ccc31b5cbd247a721c16e97fe5ec1`
- Smoke-tested official release: `v26.3.27`, `Xray-linux-64.zip` SHA2-256 `23cd9af937744d97776ee35ecad4972cf4b2109d1e0fe6be9930467608f7c8ae`
- Checked date: 2026-07-13
- Relevant source files in upstream checkout:
  - `main/version.go`
  - `main/run.go`
  - `main/main.go`
  - `main/commands/base/execute.go`

## Version command

Use program and arguments separately, without a shell:

```text
xray version
```

Observed/expected behavior:

- Exit code: `0` on success.
- Stdout: multi-line version statement. The first line starts with `Xray ` followed by a semantic version, for example:

```text
Xray 25.6.8 (Xray, Penetrates Everything.) Custom
A unified platform for anti-censorship.
```

- Stderr: normally empty for `version`; the smoke-tested Linux release wrote the version statement to stdout only.
- Legacy compatibility: `xray -version` is still translated to `xray version` by `main/main.go`, but the runner uses the modern subcommand form.

## Config test command

Use program and arguments separately, without a shell:

```text
xray run -test -config /path/to/config.json
```

The current upstream parser also accepts `-c` as a short alias and accepts flags through the Go flag parser for the `run` command. The runner uses the explicit long `-config` spelling to avoid ambiguity.

Observed/expected behavior:

- Valid config:
  - Exit code: `0`.
  - The command exits after loading and validating the config; it does not keep the proxy process running.
  - Stdout includes the version banner and may include informational log lines plus `Configuration OK.` depending on config and build; callers must preserve stdout and stderr separately.
- Invalid JSON/config:
  - Exit code: non-zero. A real release smoke test with invalid JSON returned `23`.
  - The smoke-tested release emitted the version banner and detailed startup failure to stdout while stderr was empty, so callers must inspect both streams and must not assume all validation errors are on stderr.
- Missing/unreadable config:
  - Exit code: non-zero.
  - Error details are emitted to stderr.

## Timeout and process termination contract

The runner must:

- start Xray with `QProcess::start(program, arguments)`;
- never use `system()`, `popen()`, shell scripts, or a combined command string;
- keep stdout and stderr separate;
- preserve the raw stdout/stderr bytes;
- report `exitCode`, `exitStatus`, startup error text, and timeout status;
- on timeout, call `terminate()`, wait briefly, then call `kill()` if the child is still running; callers must inspect the final process state and termination error because cleanup failure is reported instead of being assumed impossible.

## Differences from old Nekoray Xray/v2ray integration

Useful old parts from Nekoray 3.26:

- `CoreProcess`/`ExternalProcess` already centralize `QProcess` start, stdout/stderr observation, crash state, and kill semantics.
- The old core process injected Xray asset-related environment variables such as `XRAY_LOCATION_ASSET` when `geoip.dat` was found.
- The old startup path waited for a recognizable readiness message before marking the core as running.

Outdated or not reused in this stage:

- The old path was a full runtime integration tied to profile startup, generated configs, gRPC readiness, restart behavior, and UI state.
- Old environment toggles and generated V2Ray/Xray routing behavior are not part of the isolated runner.
- Old command assumptions must not replace the current upstream contract; this stage uses `xray version` and `xray run -test -config ...` as verified above.

## Stage-one limitations

This first stage adds only an isolated binary runner. It deliberately does not add:

- VLESS/VMess/Trojan/Shadowsocks generation;
- XHTTP, REALITY, Hysteria2, or WireGuard support;
- profile, subscription, TUN, VPN, GUI, or runtime flow integration;
- a core selection UI;
- Xray download/update logic;
- PATH auto-discovery;
- Xray binaries in the Git repository;
- changes to the existing sing-box/`nekobox_core` behavior.
