#include "constants.h"
#include "utils.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QUrl>
#include <QFileInfo>
#include <QProcess>
#include <array>

// ─────────────────────────────────────────────
//  STATUS HELPERS
// ─────────────────────────────────────────────
QString statusToString(DownloadStatus s) {
    switch (s) {
        case DownloadStatus::Queued:     return QStringLiteral("Queued");
        case DownloadStatus::Starting:   return QStringLiteral("Starting");
        case DownloadStatus::Running:    return QStringLiteral("Downloading");
        case DownloadStatus::Paused:     return QStringLiteral("Paused");
        case DownloadStatus::Cancelling: return QStringLiteral("Cancelling");
        case DownloadStatus::Cancelled:  return QStringLiteral("Cancelled");
        case DownloadStatus::Completed:  return QStringLiteral("Completed");
        case DownloadStatus::Failed:     return QStringLiteral("Failed");
    }
    return QStringLiteral("Queued");
}

DownloadStatus statusFromString(const QString& s) {
    if (s == u"Queued")      return DownloadStatus::Queued;
    if (s == u"Starting")    return DownloadStatus::Starting;
    if (s == u"Downloading") return DownloadStatus::Running;
    if (s == u"Paused")      return DownloadStatus::Paused;
    if (s == u"Cancelling")  return DownloadStatus::Cancelling;
    if (s == u"Cancelled")   return DownloadStatus::Cancelled;
    if (s == u"Completed")   return DownloadStatus::Completed;
    if (s == u"Failed")      return DownloadStatus::Failed;
    return DownloadStatus::Queued;
}

bool isTerminal(DownloadStatus s) {
    return s == DownloadStatus::Completed
        || s == DownloadStatus::Failed
        || s == DownloadStatus::Cancelled;
}

bool canTransition(DownloadStatus from, DownloadStatus to) {
    using DS = DownloadStatus;
    switch (from) {
        case DS::Queued:
            return to == DS::Starting || to == DS::Cancelled;
        case DS::Starting:
            return to == DS::Running || to == DS::Failed || to == DS::Cancelled;
        case DS::Running:
            return to == DS::Paused || to == DS::Cancelling
                || to == DS::Completed || to == DS::Failed;
        case DS::Paused:
            return to == DS::Queued || to == DS::Cancelling || to == DS::Cancelled;
        case DS::Cancelling:
            return to == DS::Cancelled || to == DS::Failed;
        case DS::Failed:
            return to == DS::Queued;
        default:
            return false;
    }
}

bool safeTransition(DownloadStatus& current, DownloadStatus to) {
    if (canTransition(current, to)) {
        current = to;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────
//  FORMAT OPTIONS
// ─────────────────────────────────────────────
const QList<FormatOption>& allFormatOptions() {
    static const QList<FormatOption> opts = {
        { u"Best Quality (auto)"_qs,  u"bv*+ba/b"_qs },
        { u"Worst Quality (auto)"_qs, u"worst"_qs },
        { u"MP4 - 1080p"_qs,
          u"bv*[height<=1080][ext=mp4]+ba[ext=m4a]/bv*[height<=1080]+ba/b[height<=1080]"_qs },
        { u"MP4 - 720p"_qs,
          u"bv*[height<=720][ext=mp4]+ba[ext=m4a]/bv*[height<=720]+ba/b[height<=720]"_qs },
        { u"MP4 - 480p"_qs,
          u"bv*[height<=480][ext=mp4]+ba[ext=m4a]/bv*[height<=480]+ba/b[height<=480]"_qs },
        { u"MP4 - 360p"_qs,
          u"bv*[height<=360][ext=mp4]+ba[ext=m4a]/bv*[height<=360]+ba/b[height<=360]"_qs },
        { u"Audio Only - MP3"_qs,     u"ba/b"_qs },
        { u"Audio Only - M4A"_qs,     u"ba[ext=m4a]/ba/b"_qs },
    };
    return opts;
}

QString formatValueForKey(const QString& key) {
    for (const auto& opt : allFormatOptions())
        if (opt.label == key) return opt.value;
    return {};
}

bool isValidFormatKey(const QString& key) {
    for (const auto& opt : allFormatOptions())
        if (opt.label == key) return true;
    return false;
}

QString qualityMapLookup(const QString& quality) {
    static const QMap<QString, QString> qmap = {
        { u"best"_qs,  u"Best Quality (auto)"_qs },
        { u"1080p"_qs, u"MP4 - 1080p"_qs },
        { u"720p"_qs,  u"MP4 - 720p"_qs },
        { u"480p"_qs,  u"MP4 - 480p"_qs },
        { u"360p"_qs,  u"MP4 - 360p"_qs },
        { u"audio"_qs, u"Audio Only - MP3"_qs },
        { u"mp3"_qs,   u"Audio Only - MP3"_qs },
        { u"m4a"_qs,   u"Audio Only - M4A"_qs },
        { u"worst"_qs, u"Worst Quality (auto)"_qs },
    };
    return qmap.value(quality.toLower(), u"Best Quality (auto)"_qs);
}

// ─────────────────────────────────────────────
//  CATEGORY MAP
// ─────────────────────────────────────────────
static const QMap<QString, QString>& categoryMap() {
    static const QMap<QString, QString> m = {
        // Video
        { u".mp4"_qs,  u"Videos"_qs }, { u".mkv"_qs,  u"Videos"_qs },
        { u".avi"_qs,  u"Videos"_qs }, { u".mov"_qs,  u"Videos"_qs },
        { u".wmv"_qs,  u"Videos"_qs }, { u".flv"_qs,  u"Videos"_qs },
        { u".webm"_qs, u"Videos"_qs }, { u".m4v"_qs,  u"Videos"_qs },
        { u".ts"_qs,   u"Videos"_qs }, { u".3gp"_qs,  u"Videos"_qs },
        // Audio
        { u".mp3"_qs,  u"Music"_qs  }, { u".m4a"_qs,  u"Music"_qs  },
        { u".flac"_qs, u"Music"_qs  }, { u".ogg"_qs,  u"Music"_qs  },
        { u".wav"_qs,  u"Music"_qs  }, { u".aac"_qs,  u"Music"_qs  },
        { u".opus"_qs, u"Music"_qs  }, { u".wma"_qs,  u"Music"_qs  },
        // Documents
        { u".pdf"_qs,  u"Documents"_qs }, { u".docx"_qs, u"Documents"_qs },
        { u".doc"_qs,  u"Documents"_qs }, { u".xlsx"_qs, u"Documents"_qs },
        { u".xls"_qs,  u"Documents"_qs }, { u".pptx"_qs, u"Documents"_qs },
        { u".ppt"_qs,  u"Documents"_qs }, { u".txt"_qs,  u"Documents"_qs },
        { u".epub"_qs, u"Documents"_qs }, { u".csv"_qs,  u"Documents"_qs },
        { u".rtf"_qs,  u"Documents"_qs },
        // Archives
        { u".zip"_qs,  u"Archives"_qs }, { u".rar"_qs,  u"Archives"_qs },
        { u".7z"_qs,   u"Archives"_qs }, { u".tar"_qs,  u"Archives"_qs },
        { u".gz"_qs,   u"Archives"_qs }, { u".bz2"_qs,  u"Archives"_qs },
        { u".xz"_qs,   u"Archives"_qs }, { u".iso"_qs,  u"Archives"_qs },
        // Programs
        { u".exe"_qs,  u"Programs"_qs }, { u".msi"_qs,  u"Programs"_qs },
        { u".apk"_qs,  u"Programs"_qs }, { u".dmg"_qs,  u"Programs"_qs },
        { u".deb"_qs,  u"Programs"_qs }, { u".rpm"_qs,  u"Programs"_qs },
        { u".pkg"_qs,  u"Programs"_qs }, { u".bin"_qs,  u"Programs"_qs },
        { u".img"_qs,  u"Programs"_qs },
    };
    return m;
}

QString categoryForExtension(const QString& ext) {
    return categoryMap().value(ext.toLower(), u"Other"_qs);
}

QString categoryForFormatKey(const QString& fmtKey) {
    if (fmtKey.contains(u"Audio")) return u"Music"_qs;
    if (fmtKey.contains(u"1080p") || fmtKey.contains(u"720p") ||
        fmtKey.contains(u"480p")  || fmtKey.contains(u"360p") ||
        fmtKey.contains(u"MP4")   || fmtKey.contains(u"Best"))
        return u"Videos"_qs;
    return u"Other"_qs;
}

QString makeCategorizedOutputDir(const QString& baseDir, const QString& filename, const QString& fmtKey) {
    QString category;
    if (!filename.isEmpty()) {
        QString ext = QFileInfo(filename).suffix().toLower();
        if (!ext.isEmpty()) ext = '.' + ext;
        category = categoryForExtension(ext);
    } else {
        category = categoryForFormatKey(fmtKey);
    }
    QString catDir = QDir(baseDir).filePath(category);
    QDir().mkpath(catDir);
    return catDir;
}

// ─────────────────────────────────────────────
//  YTDLP DOMAIN DETECTION
// ─────────────────────────────────────────────
static const QSet<QString>& ytdlpDomains() {
    static const QSet<QString> d = {
        u"youtube.com"_qs, u"youtu.be"_qs, u"vimeo.com"_qs,
        u"dailymotion.com"_qs, u"tiktok.com"_qs, u"soundcloud.com"_qs
    };
    return d;
}

static const QSet<QString>& directExtensions() {
    static const QSet<QString> e = {
        u".exe"_qs, u".msi"_qs, u".zip"_qs, u".rar"_qs, u".7z"_qs,
        u".iso"_qs, u".apk"_qs, u".dmg"_qs, u".tar"_qs, u".gz"_qs,
        u".bz2"_qs, u".xz"_qs, u".zst"_qs, u".deb"_qs, u".rpm"_qs,
        u".pkg"_qs, u".bin"_qs, u".img"_qs, u".pdf"_qs, u".docx"_qs,
        u".doc"_qs, u".xlsx"_qs, u".xls"_qs, u".pptx"_qs, u".ppt"_qs,
        u".mp3"_qs, u".wav"_qs, u".flac"_qs, u".ogg"_qs, u".aac"_qs,
        u".opus"_qs, u".wma"_qs, u".m4a"_qs, u".mp4"_qs, u".mkv"_qs,
        u".avi"_qs, u".mov"_qs, u".wmv"_qs, u".flv"_qs, u".webm"_qs,
        u".m4v"_qs, u".ts"_qs, u".3gp"_qs
    };
    return e;
}

bool shouldUseYtdlp(const QString& url) {
    QUrl u(url);
    if (!u.isValid()) return false;

    // Rule 1: direct file extension guard
    QString path = u.path().toLower();
    int qmark = path.indexOf('?');
    if (qmark >= 0) path = path.left(qmark);
    QString ext = QFileInfo(path).suffix();
    if (!ext.isEmpty() && directExtensions().contains('.' + ext))
        return false;

    // Rule 2: known video/audio platform
    QString host = u.host().toLower();
    if (host.startsWith(u"www.")) host = host.mid(4);
    if (host.startsWith(u"m."))   host = host.mid(2);
    for (const QString& domain : ytdlpDomains()) {
        if (host == domain || host.endsWith('.' + domain))
            return true;
    }
    return false;
}

bool isDirectFileUrl(const QString& url) {
    QUrl u(url);
    if (!u.isValid()) return false;
    QString path = u.path().toLower();
    int qmark = path.indexOf('?');
    if (qmark >= 0) path = path.left(qmark);
    QString ext = '.' + QFileInfo(path).suffix();
    // DIRECT_FILE_EXTENSIONS from Python
    static const QSet<QString> dfe = {
        u".exe"_qs, u".msi"_qs, u".zip"_qs, u".rar"_qs, u".7z"_qs,
        u".iso"_qs, u".pdf"_qs, u".apk"_qs, u".dmg"_qs, u".tar"_qs,
        u".gz"_qs, u".bz2"_qs, u".xz"_qs, u".docx"_qs, u".xlsx"_qs,
        u".pptx"_qs, u".doc"_qs, u".xls"_qs, u".ppt"_qs, u".deb"_qs,
        u".rpm"_qs, u".pkg"_qs, u".bin"_qs, u".img"_qs
    };
    return dfe.contains(ext);
}

QString getGenericFileLabel(const QString& url, const QString& filename) {
    static const QMap<QString, QString> labels = {
        { u".exe"_qs,  u"Software (.exe)"_qs },
        { u".msi"_qs,  u"Installer (.msi)"_qs },
        { u".apk"_qs,  u"Android Package (.apk)"_qs },
        { u".dmg"_qs,  u"macOS Disk Image (.dmg)"_qs },
        { u".deb"_qs,  u"Debian Package (.deb)"_qs },
        { u".rpm"_qs,  u"RPM Package (.rpm)"_qs },
        { u".pkg"_qs,  u"Package (.pkg)"_qs },
        { u".bin"_qs,  u"Binary (.bin)"_qs },
        { u".img"_qs,  u"Disk Image (.img)"_qs },
        { u".zip"_qs,  u"Archive (.zip)"_qs },
        { u".rar"_qs,  u"Archive (.rar)"_qs },
        { u".7z"_qs,   u"Archive (.7z)"_qs },
        { u".tar"_qs,  u"Archive (.tar)"_qs },
        { u".gz"_qs,   u"Archive (.gz)"_qs },
        { u".bz2"_qs,  u"Archive (.bz2)"_qs },
        { u".xz"_qs,   u"Archive (.xz)"_qs },
        { u".zst"_qs,  u"Archive (.zst)"_qs },
        { u".iso"_qs,  u"Disk Image (.iso)"_qs },
        { u".pdf"_qs,  u"PDF Document"_qs },
        { u".docx"_qs, u"Word Document (.docx)"_qs },
        { u".doc"_qs,  u"Word Document (.doc)"_qs },
        { u".xlsx"_qs, u"Spreadsheet (.xlsx)"_qs },
        { u".xls"_qs,  u"Spreadsheet (.xls)"_qs },
        { u".pptx"_qs, u"Presentation (.pptx)"_qs },
        { u".ppt"_qs,  u"Presentation (.ppt)"_qs },
        { u".txt"_qs,  u"Text File (.txt)"_qs },
        { u".csv"_qs,  u"CSV File (.csv)"_qs },
        { u".epub"_qs, u"eBook (.epub)"_qs },
        { u".mp3"_qs,  u"Audio File (.mp3)"_qs },
        { u".wav"_qs,  u"Audio File (.wav)"_qs },
        { u".flac"_qs, u"Audio File (.flac)"_qs },
        { u".ogg"_qs,  u"Audio File (.ogg)"_qs },
        { u".aac"_qs,  u"Audio File (.aac)"_qs },
        { u".opus"_qs, u"Audio File (.opus)"_qs },
        { u".wma"_qs,  u"Audio File (.wma)"_qs },
        { u".m4a"_qs,  u"Audio File (.m4a)"_qs },
        { u".mp4"_qs,  u"Video File (.mp4)"_qs },
        { u".mkv"_qs,  u"Video File (.mkv)"_qs },
        { u".avi"_qs,  u"Video File (.avi)"_qs },
        { u".mov"_qs,  u"Video File (.mov)"_qs },
        { u".wmv"_qs,  u"Video File (.wmv)"_qs },
        { u".flv"_qs,  u"Video File (.flv)"_qs },
        { u".webm"_qs, u"Video File (.webm)"_qs },
        { u".m4v"_qs,  u"Video File (.m4v)"_qs },
        { u".ts"_qs,   u"Video File (.ts)"_qs },
        { u".3gp"_qs,  u"Video File (.3gp)"_qs },
    };

    auto tryPath = [&](const QString& candidate, bool isUrl) -> QString {
        QString p = isUrl ? QUrl(candidate).path() : candidate;
        int q = p.indexOf('?');
        if (q >= 0) p = p.left(q);
        int h = p.indexOf('#');
        if (h >= 0) p = p.left(h);
        QString ext = '.' + QFileInfo(p.toLower()).suffix();
        if (labels.contains(ext)) return labels.value(ext);
        if (ext.length() > 1) return QStringLiteral("Generic File (%1)").arg(ext);
        return {};
    };

    if (!filename.isEmpty()) {
        QString r = tryPath(filename, false);
        if (!r.isEmpty()) return r;
    }
    if (!url.isEmpty()) {
        QString r = tryPath(url, true);
        if (!r.isEmpty()) return r;
    }
    return QStringLiteral("Generic File");
}

// ─────────────────────────────────────────────
//  APP PATHS
// ─────────────────────────────────────────────
AppPaths& appPaths() {
    static AppPaths p;
    return p;
}

void initAppPaths() {
    AppPaths& p = appPaths();
    p.scriptDir = QCoreApplication::applicationDirPath();
    p.iconsDir  = QDir(p.scriptDir).filePath(u"icons"_qs);

    p.historyFile       = QDir(p.scriptDir).filePath(u"history.json"_qs);
    p.queueDbFile       = QDir(p.scriptDir).filePath(u"queue_db.json"_qs);
    p.scheduleFile      = QDir(p.scriptDir).filePath(u"schedule.json"_qs);
    p.apiTokenFile      = QDir(p.scriptDir).filePath(u".ydm_api_token"_qs);
    p.ffmpegDeployPath  = QDir(p.scriptDir).filePath(u"ffmpeg.exe"_qs);

    // Locate yt-dlp: prefer "yt-dlp" binary on PATH, else python -m yt_dlp
    QString ytdlpBin = QStandardPaths::findExecutable(u"yt-dlp"_qs);
    if (!ytdlpBin.isEmpty()) {
        p.ytdlpCmd = { ytdlpBin };
    } else {
        // Try python / python3
        QString pyBin = QStandardPaths::findExecutable(u"python"_qs);
        if (pyBin.isEmpty()) pyBin = QStandardPaths::findExecutable(u"python3"_qs);
        if (pyBin.isEmpty()) pyBin = u"python"_qs;
        p.ytdlpCmd = { pyBin, u"-m"_qs, u"yt_dlp"_qs };
    }
}
