#pragma once
#include <QString>
#include <QMap>
#include <QSet>
#include <QStringList>

// ─────────────────────────────────────────────
//  DOWNLOAD STATUS ENUM
// ─────────────────────────────────────────────
enum class DownloadStatus {
    Queued,
    Starting,
    Running,
    Paused,
    Cancelling,
    Cancelled,
    Completed,
    Failed
};

QString statusToString(DownloadStatus s);
DownloadStatus statusFromString(const QString& s);
bool isTerminal(DownloadStatus s);
bool canTransition(DownloadStatus from, DownloadStatus to);
bool safeTransition(DownloadStatus& current, DownloadStatus to);

// ─────────────────────────────────────────────
//  FORMAT OPTIONS
// ─────────────────────────────────────────────
// Ordered list of (label, yt-dlp format string) pairs
struct FormatOption {
    QString label;
    QString value;
};

// Returns the ordered list of all format options
const QList<FormatOption>& allFormatOptions();

// Returns yt-dlp format string for a given label key.
// Returns empty string if not found.
QString formatValueForKey(const QString& key);

// Returns true if key is a valid format label
bool isValidFormatKey(const QString& key);

// Map from API quality shorthand → format label
QString qualityMapLookup(const QString& quality);

// ─────────────────────────────────────────────
//  CATEGORY / FILE-TYPE MAPS
// ─────────────────────────────────────────────
QString categoryForExtension(const QString& ext);
QString categoryForFormatKey(const QString& fmtKey);
QString makeCategorizedOutputDir(const QString& baseDir, const QString& filename, const QString& fmtKey = {});

// ─────────────────────────────────────────────
//  YTDLP DOMAIN + EXTENSION SETS
// ─────────────────────────────────────────────
bool shouldUseYtdlp(const QString& url);
bool isDirectFileUrl(const QString& url);
QString getGenericFileLabel(const QString& url, const QString& filename = {});

// ─────────────────────────────────────────────
//  RUNTIME PATHS (set at startup)
// ─────────────────────────────────────────────
struct AppPaths {
    QString scriptDir;        // next to the executable
    QString iconsDir;
    QString historyFile;
    QString queueDbFile;
    QString scheduleFile;
    QString apiTokenFile;
    QString ffmpegDeployPath;
    QStringList ytdlpCmd;     // e.g. ["python", "-m", "yt_dlp"]
};

AppPaths& appPaths();          // returns the singleton (call initAppPaths() first)
void initAppPaths();            // must be called before any AppPaths access

// ─────────────────────────────────────────────
//  NETWORK / API CONSTANTS
// ─────────────────────────────────────────────
constexpr int    API_PORT            = 8765;
constexpr int    INSTANCE_PORT       = API_PORT + 1;  // 8766
constexpr qint64 API_MAX_BODY        = 32 * 1024;
constexpr qint64 GENERIC_MAX_FILE_BYTES = 50LL * 1024 * 1024 * 1024;
constexpr int    HISTORY_MAX_BYTES   = 4 * 1024 * 1024;
constexpr int    QUEUE_DB_MAX_BYTES  = 2 * 1024 * 1024;
constexpr int    HISTORY_MAX_ENTRIES = 5000;
constexpr int    QUEUE_MAX_ENTRIES   = 2000;
constexpr int    MAX_CONCURRENT      = 2;

inline const char* API_HOST            = "127.0.0.1";
inline const char* INSTANCE_MAGIC      = "RAINAX_SHOW_WINDOW\n";
