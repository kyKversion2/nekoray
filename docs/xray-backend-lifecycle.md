# Xray backend lifecycle

This document describes the isolated Xray runtime backend added for raw JSON configuration execution. It is intentionally not wired into the GUI profile flow yet.

## State model

`XrayBackend` exposes a minimal lifecycle with the following states:

- `Stopped`: no runtime process is owned by the backend.
- `Validating`: pre-start binary and raw JSON config validation is running through `XrayCoreRunner`.
- `Starting`: validation passed and `QProcess::start()` has been requested for the runtime process.
- `Running`: Qt reported that the runtime process started successfully.
- `Stopping`: a caller requested process termination.
- `Crashed`: the runtime process exited when no backend stop was requested.

No state is based on Xray readiness text. Stdout may contain useful readiness lines, but they are treated only as process output.

## Validation flow

Before every `start(configPath)` call the backend:

1. validates that the configured Xray binary exists, is a file, and is executable;
2. validates that the config path exists and is a file;
3. calls `XrayCoreRunner::testConfig(configPath)`;
4. rejects startup if the test process fails to start, times out, exits non-zero, crashes, or cannot be cleaned up.

The backend never logs or emits the full config JSON content as part of validation errors.

## Start flow

Runtime startup uses the modern Xray CLI form:

```text
xray run -config <config-path>
```

The executable path and arguments are passed separately to `QProcess`; no shell is used. A second `start()` on an already running backend instance is rejected. Successful startup means that config validation passed and Qt reported that the process started; it does not depend on a specific stdout/stderr line.

## Stop flow

`stop()` is bounded:

1. mark the lifecycle as a requested stop;
2. call `terminate()`;
3. wait for a bounded graceful shutdown interval;
4. call `kill()` if the process is still alive;
5. wait for a bounded forced shutdown interval;
6. return an explicit error if the process is still not `NotRunning`.

The destructor uses the same bounded cleanup policy so it does not intentionally leave an Xray process running.

## Crash flow

If the runtime process reaches `NotRunning` without a requested stop, the backend transitions to `Crashed` and emits `crashed(exitCode, exitStatus)`. Requested stops emit `stopped(exitCode, exitStatus)` instead.

## Stdout and stderr

The backend keeps stdout and stderr separate by using `QProcess::SeparateChannels`. Output is forwarded through separate Qt signals. The backend does not parse readiness or validation success from runtime output.

## Timeout policy

All blocking waits use finite timeouts. Config validation uses `XrayCoreRunner` bounded waits. Runtime stop first waits for graceful termination and then for forced termination. There are no infinite waits in backend lifecycle methods.

## Stage limitations

This stage only supports launching an already prepared raw Xray JSON config. It deliberately does not add protocol generation, subscription parsing, XHTTP UI, TUN, Core Manager, Xray download/update logic, GUI Xray selection, or integration with existing sing-box/nekobox runtime flows.
