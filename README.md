# OneClone

A lightweight Windows system-tray application for managing [rclone](https://rclone.org/) jobs — mounts, syncs, and copies — without touching the command line.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Qt](https://img.shields.io/badge/Qt-6-green)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

---

## Features

- **Mount, sync, and copy** — start and stop rclone jobs from a simple UI
- **System tray integration** — runs in the background; accessible from the tray at all times
- **Single-instance enforcement** — launching a second instance brings the existing window to focus
- **Autostart support** — optional Windows registry entry to launch the app on login
- **Live output log** — per-job console output streamed in real time
- **Progress tracking** — transfer speed, bytes transferred, percentage, and ETA for sync/copy jobs
- **Persistent settings** — all jobs and shared settings saved to a local `settings.json` file
- **Graceful rclone shutdown** — sends Ctrl+C to rclone before falling back to a hard kill

---

## Requirements

- **Windows 10 or later**
- **Qt 6** (tested with Qt 6.x; Qt 5.15 may work but is untested)
- **rclone** installed and accessible — [download here](https://rclone.org/downloads/)
- A C++17-capable compiler (MSVC 2019+ or MinGW-w64)

---

## Project Structure

```
OneClone/
├── main.cpp                    # Entry point; single-instance enforcement
├── model/
│   ├── Config.h                # Compile-time constants (app name, regex, limits)
│   ├── Job.h / Job.cpp         # Individual rclone job: lifecycle, process, output parsing
│   ├── Manager.h / Manager.cpp # Owns all jobs and shared settings; load/save
│   ├── SharedSettings.h        # Settings shared across all jobs (rclone path, cache, etc.)
│   └── SharedSettings.cpp
└── view/
    ├── MainWindow.h / .cpp     # Main window: settings tab, job list tab, job detail tab
    ├── MainWindow.ui           # Qt Designer layout for MainWindow
    ├── JobWidget.h / .cpp      # Per-job widget shown in the job list
    └── JobWidget.ui            # Qt Designer layout for JobWidget
```

---

## Configuration

All settings are stored in `settings.json` next to the application executable. The file is created automatically on first launch with default values.

### Shared settings

| Setting               | Default              | Description                                  |
|-----------------------|----------------------|----------------------------------------------|
| `rclonePath`          | `C:\RClone\rclone.exe` | Path to the rclone executable              |
| `cacheMode`           | `full`               | VFS cache mode (`off`, `minimal`, `writes`, `full`) |
| `cacheMaxSize`        | `50` GB              | Maximum VFS cache size                       |
| `cacheMinFreeSpace`   | `50` GB              | Minimum free disk space before cache evicts  |
| `cacheMaxAge`         | `24` hours           | Maximum age of cached files                  |
| `readChunkSize`       | `128` MB             | Initial VFS read chunk size                  |
| `readChunkSizeLimit`  | `2048` MB            | Maximum VFS read chunk size                  |
| `bufferSize`          | `256` MB             | rclone in-memory buffer size                 |
| `transfers`           | `16`                 | Number of parallel file transfers            |
| `checkers`            | `16`                 | Number of parallel checkers                  |
| `links`               | `true`               | Follow symlinks (`--links` flag)             |

### Per-job settings

| Field       | Description                                              |
|-------------|----------------------------------------------------------|
| `name`      | Display name shown in the UI                             |
| `type`      | Job type: `mount`, `sync`, or `copy`                     |
| `local`     | Local path (folder or mount point)                       |
| `remote`    | rclone remote path (e.g. `gdrive:backup`)                |
| `autostart` | Start this job automatically when the app launches       |
| `readOnly`  | Mount in read-only mode (mount jobs only)                |

---

## Usage

### First run

1. Launch `OneClone.exe`.
2. Go to the **Settings** tab and set the path to `rclone.exe`.
3. Adjust shared transfer settings as needed and click **Save**.

### Adding a job

1. Go to the **Jobs** tab and click **Add**.
2. Fill in the job name, type, local path, and remote.
3. Click **Save**. The job appears in the job list.

### Starting and stopping

- Click **Mount / Sync ▲ / Copy ▲** to start a job in the default direction.
- For `sync` and `copy` jobs, the **▼** button starts the job with source and destination swapped.
- Click **Stop** to gracefully shut down the running rclone process.
- Jobs can also be toggled directly from the system tray context menu.

### Output log

Double-click any job in the list to open its detail view, which includes a live output log of everything rclone prints to stdout/stderr.

### Startup with Windows

Check **Launch at startup** in the Settings tab to register the app in the Windows registry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`. The app will launch minimised to the tray (`--tray` flag) on login.

---

## Status indicators

| Status     | Meaning                                                  |
|------------|----------------------------------------------------------|
| `Stopped`  | The rclone process is not running                        |
| `Starting` | Process launched; waiting for first output (or mount ready signal) |
| `Running`  | Process is active and producing output                   |
| `Stopping` | Graceful shutdown in progress                            |
| `Errored`  | An error pattern was detected in output, or the process exited with a non-zero code |

For `mount` jobs, the transition from `Starting` to `Running` occurs when rclone prints:
```
The service rclone has been started.
```

For `sync` and `copy` jobs, any output from rclone triggers the `Running` state.

---

## Error detection

Rclone output is scanned against the following regular expression after each line:

```
NOTICE:.*failed|ERROR:
```

If a match is found, the job transitions to `Errored` and rclone is terminated. You can adjust this pattern in `model/Config.h` (`DEFAULT_ERROR_REGEX`).

---

## Known limitations

- **Windows only.** The graceful stop mechanism uses Win32 APIs (`AttachConsole`, `GenerateConsoleCtrlEvent`) that have no cross-platform equivalent.
- **rclone must be pre-configured.** OneClone does not manage rclone remotes — use `rclone config` in a terminal to set up remotes before using them here.
- **Mount jobs require WinFsp.** rclone FUSE mounts on Windows depend on [WinFsp](https://winfsp.dev/) being installed.
- **No validation on paths or remotes.** Incorrect paths will result in rclone failing at start time; check the output log for details.

---

## License

MIT — see [LICENSE](LICENSE) for details.
