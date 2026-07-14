# Raw Xray JSON profiles

Raw Xray profiles are the first application-startable Xray profile type. They are intentionally limited to complete user-provided Xray JSON and do not add protocol parsers, subscription parsing, generated Xray fields, TUN, system proxy integration, traffic stats, or speed tests.

## Marker and storage

The stable profile marker is `xray-raw`. It is stored in `CustomBean::core`; the `ProxyEntity::type` remains `custom`. The complete Xray JSON config is stored as raw text in `CustomBean::config_simple`.

## Raw pass-through rule

The GUI/runtime may parse the raw text only to verify that it is syntactically valid JSON and that the root is a JSON object. The parsed document is never serialized back for runtime. The exact UTF-8 bytes from `CustomBean::config_simple` are written to the temporary config file and passed to Xray unchanged. Unknown fields, key order, formatting, and number/string representation are not normalized by Nekoray.

## Xray binary resolution

The Xray executable path comes only from the existing Extra Core setting with key `xray`:

```cpp
NekoGui::dataStore->extraCore->Get("xray")
```

There is no PATH lookup, downloader, updater, or Core Manager UI in this stage.

## Start flow

`MainWindow::neko_start` detects the raw Xray marker before calling `BuildConfig`. It keeps the existing start/stop serialization, stops any currently running profile, resolves Extra Core `xray`, creates an `XrayProfileSession` in the core dispatcher thread, writes the raw temporary config, validates it through `XrayBackend`/`XrayCoreRunner`, starts Xray through `XrayBackend`, then updates `started_id`, `running`, and the UI. The raw JSON is not sent to nekobox_core gRPC and is not launched through `ExternalProcess`.

## Stop flow

`MainWindow::neko_stop` detects when the running profile is raw Xray and calls `XrayProfileSession::stop`. It does not call gRPC Stop and does not kill the Xray process through `ExternalProcess`. After bounded stop, the session is deleted, `started_id` is reset, `running` is cleared, and the UI is refreshed.

## Crash flow

`XrayProfileSession` forwards `XrayBackend::crashed`. The main window clears the session, resets `started_id`, clears `running`, refreshes UI, and logs only exit code/status. It does not log config content and does not auto-restart.

## Temporary file lifetime

`XrayProfileSession` owns a `QTemporaryFile`. The file is created with an unpredictable Qt temporary name, receives the original raw UTF-8 bytes, and remains alive after `QProcess::waitForStarted` for the whole Xray runtime lifecycle. It is removed after normal stop, failed start, crash, or session destruction.

## Thread ownership

`XrayProfileSession` owns `XrayBackend`, which owns `QProcess`. The main window creates and starts/stops the session in the existing `DS_cores` dispatcher thread. Lifecycle methods assert current-thread ownership.

## BuildConfig bypass and guard

Raw Xray profiles bypass `BuildConfig` because `BuildConfigSingBox` produces sing-box JSON and applies sing-box-only merging/routing/TUN behavior. If a raw Xray profile reaches `BuildConfig`, it returns `Raw Xray profile must be started by the Xray runtime`. Raw Xray profiles are also rejected inside sing-box chain/front-proxy BuildConfig paths.

## Current limitations

- TUN and system proxy integration are blocked for raw Xray profiles.
- Existing gRPC traffic and connection statistics are not connected to raw Xray profiles.
- TcpPing, UrlTest, and FullTest speed tests are skipped as unsupported.
- Subscription parsing and protocol URI parsing are not implemented.
