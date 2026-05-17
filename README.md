# RAINAX – C++20 / Qt6 Download Manager

A complete 1:1 port of the PyQt6 RAINAX application to C++20 using Qt6 Widgets.

---

## Project Layout

```
rainax/
├── CMakeLists.txt
├── resources/
│   └── resources.qrc          ← Qt resource bundle (icons)
├── include/                    ← All headers
│   ├── constants.h             ← Enums, format map, paths, limits
│   ├── downloaditem.h          ← Data model (POD struct)
│   ├── downloadworker.h        ← yt-dlp QThread worker
│   ├── genericfileworker.h     ← HTTP range-request downloader
│   ├── downloadmanager.h       ← Queue, concurrency, signals
│   ├── persistence.h           ← History / queue / schedule JSON
│   ├── scheduleitem.h          ← Scheduled-download model
│   ├── apiserver.h             ← Local HTTP API (port 8765)
│   ├── instanceserver.h        ← Single-instance TCP signalling
│   ├── themeprovider.h         ← Dark/light palette + QSS builder
│   ├── iconhelper.h            ← Safe icon loading + cache
│   ├── utils.h                 ← Security, formatting, paths
│   ├── ytdlpupdateworker.h
│   ├── ffmpegupdateworker.h
│   └── widgets/
│       ├── statusbadge.h       ← Pill-shaped status label
│       ├── glossyprogressbar.h ← Animated gradient progress bar
│       ├── statchip.h          ← Speed/ETA chip
│       ├── statbox.h           ← Header stat card
│       ├── sidebarbutton.h     ← Checkable nav button
│       ├── ydmdialog.h         ← Themed modal dialog
│       ├── scheduledialog.h    ← Schedule-a-download dialog
│       └── backgroundloader.h  ← Async history+queue loader
└── src/
    ├── main.cpp                ← Entry point, single-instance check
    ├── constants.cpp
    ├── downloaditem.cpp
    ├── downloadworker.cpp
    ├── genericfileworker.cpp
    ├── downloadmanager.cpp
    ├── persistence.cpp
    ├── scheduleitem.cpp
    ├── apiserver.cpp
    ├── instanceserver.cpp
    ├── themeprovider.cpp
    ├── iconhelper.cpp
    ├── utils.cpp
    ├── ytdlpupdateworker.cpp
    ├── ffmpegupdateworker.cpp
    ├── mainwindow.cpp           ← Constructor, tray, startup sequence
    ├── mainwindow_build.cpp     ← buildUI(), sidebar, header, queue page
    ├── mainwindow_pages.cpp     ← History, Logs, Settings, Schedule pages
    └── mainwindow_slots.cpp     ← All slot implementations + table logic
```

---

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.22 |
| C++ compiler | MSVC 2022 / GCC 12 / Clang 15 with C++20 support |
| Qt6 | ≥ 6.5 (Core, Gui, Widgets, Network, Concurrent) |
| yt-dlp | Any recent release on PATH **or** `python -m yt_dlp` |
| ffmpeg | Optional; placed next to executable as `ffmpeg.exe` |

---

## Building on Windows (MSVC + vcpkg)

```bat
# 1. Install Qt6 via the official installer or vcpkg
#    e.g. vcpkg install qt6-base qt6-networkauth

# 2. Clone / extract source
cd C:\projects\rainax

# 3. Configure
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2022_64"

# 4. Build
cmake --build build --config Release

# 5. Run
.\build\Release\RAINAX.exe
```

## Building on Linux (GCC + system Qt6)

```bash
sudo apt install qt6-base-dev qt6-tools-dev cmake g++-12

cmake -S . -B build -DCMAKE_CXX_COMPILER=g++-12 \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/RAINAX
```

---

## Adding Icons

Place 20×20 (or larger; they are scaled) PNG files in `resources/icons/`:

```
download.png   completed.png  history.png    logs.png
settings.png   schedule.png   pause.png      resume.png
cancel.png     add.png        folder.png     tray.png
logo.png       clock.png      menu.png
```

If a file is missing, `loadIcon()` returns a transparent blank — the app still works.

---

## Architecture Notes (Python → C++ mapping)

| Python | C++ |
|---|---|
| `DownloadItem` dataclass | `struct DownloadItem` in `downloaditem.h` |
| `DownloadWorker(QRunnable)` | `DownloadWorker : QThread` |
| `GenericFileWorker(QRunnable)` | `GenericFileWorker : QThread` using `QNetworkAccessManager` |
| `DownloadManager` | `DownloadManager : QObject` (owns all items and workers) |
| `APIServer(HTTPServer)` | `APIServerThread : QThread` + `QTcpServer` |
| `APIBridge(QObject)` | `APIBridge : QObject` (cross-thread signal relay) |
| `Persistence.*` static helpers | `namespace Persistence { … }` in `persistence.cpp` |
| `ScheduledItem` dataclass | `struct ScheduledItem` in `scheduleitem.h` |
| `StatusBadge(QLabel)` | `StatusBadge : QLabel` |
| `GlossyProgressBar(QWidget)` | `GlossyProgressBar : QWidget` with `QPropertyAnimation` |
| Python `threading.Thread` | `QThread` subclasses |
| Python `queue.Queue` | Qt signals/slots + `QMutex` |
| PyQt6 `pyqtSignal` | `Q_SIGNAL` + `emit` |
| `QSettings(ini)` | `QSettings("RAINAX", "DownloadManager")` |
| Tray icon | `QSystemTrayIcon` |
| `DARK_PALETTE / LIGHT_PALETTE` dicts | `ThemeProvider::dark() / light()` → `Palette` struct |
| Style template string | `ThemeProvider::buildStyleSheet(palette)` |

### Threading model

```
GUI thread
  MainWindow
  DownloadManager  (owns items; all slot invocations land here)
  APIBridge        (receives cross-thread signals from APIServerThread)

Worker threads (one per active download)
  DownloadWorker        – wraps a yt-dlp QProcess
  GenericFileWorker     – QNetworkAccessManager HTTP loop

Server threads
  APIServerThread   – QTcpServer event loop (port 8765)
  InstanceServer    – QTcpServer event loop (port 8766)
```

All worker → GUI communication goes through **queued Qt signals** — no manual
locking required in slot bodies.

### Persistence files (next to executable)

| File | Contents |
|---|---|
| `history.json` | Audit log of all completed/failed downloads |
| `queue_db.json` | Active/paused queue (auto-restored on next launch) |
| `schedule.json` | Scheduled download entries |
| `.ydm_api_token` | 32-byte random token for the browser extension API |

### API (port 8765)

```
GET  /ping         → { "status": "running" }          (no token)
GET  /status       → { running, queued, paused, total } (X-YDM-Token required)
POST /download     → { "status": "queued" }             (X-YDM-Token required)
     body: { "url": "…", "quality": "best|1080p|720p|480p|360p|audio|mp3|m4a", "title": "…" }
```

All requests are restricted to `127.0.0.1` / `localhost`. CORS is whitelisted
for `chrome-extension://`, `moz-extension://`, and `edge-extension://` schemes.

---

## Known Differences from the Python Version

1. **ffmpeg auto-install** (`FfmpegUpdateWorker`) targets Windows only and
   uses PowerShell's `Expand-Archive`.  On Linux/macOS point `ffmpegDeployPath`
   at your system ffmpeg, or install it separately.

2. **Startup registry** (`toggleStartupRegistry`) is Windows-only.
   Equivalent systemd / launchd support can be added in the same function.

3. **Tray blinking** is approximated by toggling a CSS colour class via a
   1-second `QTimer`; the Python version uses a `QPropertyAnimation` on the
   tray icon — Qt6 desktop integration does not expose that.

4. **Browser extension**: the API is 100% compatible — the same `.crx` / `.xpi`
   extension works with both the Python and C++ versions.
