#include "utils.h"
#include "constants.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QHostInfo>
#include <QHostAddress>
#include <QSaveFile>
#include <QFile>
#include <QProcess>
#include <QRandomGenerator>
#include <QNetworkInterface>
#include <optional>

// ─────────────────────────────────────────────
//  Filename sanitization
// ─────────────────────────────────────────────
static const QRegularExpression s_badFnChars(QStringLiteral(R"([\x00-\x1f<>:"/\\|?*])"));
static const QSet<QString> s_reservedNames = {
    u"CON"_qs,  u"PRN"_qs,  u"AUX"_qs,  u"NUL"_qs,
    u"COM1"_qs, u"COM2"_qs, u"COM3"_qs, u"COM4"_qs, u"COM5"_qs,
    u"COM6"_qs, u"COM7"_qs, u"COM8"_qs, u"COM9"_qs,
    u"LPT1"_qs, u"LPT2"_qs, u"LPT3"_qs, u"LPT4"_qs, u"LPT5"_qs,
    u"LPT6"_qs, u"LPT7"_qs, u"LPT8"_qs, u"LPT9"_qs,
};

QString sanitizeFilename(const QString& name, const QString& fallback) {
    if (name.isEmpty()) return fallback;

    // Take last path component, URL-decode
    QString n = QFileInfo(name.contains('\\') ? name.replace('\\', '/') : name)
                    .fileName();
    n = QUrl::fromPercentEncoding(n.toUtf8());
    n = n.replace(s_badFnChars, u"_"_qs);

    // Strip leading . - SPACE
    while (!n.isEmpty() && (n.front() == '.' || n.front() == '-' || n.front() == ' '))
        n = n.mid(1);

    // Reserved device names
    QString stem = QFileInfo(n).completeBaseName().toUpper();
    if (s_reservedNames.contains(stem))
        n = '_' + n;

    // Truncate to 200 chars
    if (n.length() > 200) {
        QString ext = QFileInfo(n).suffix();
        QString base = QFileInfo(n).completeBaseName();
        n = base.left(200 - ext.length() - 1) + '.' + ext;
    }

    return n.isEmpty() ? fallback : n;
}

// ─────────────────────────────────────────────
//  Path traversal guard
// ─────────────────────────────────────────────
std::optional<QString> safeJoin(const QString& baseDir, const QString& part) {
    try {
        QString baseReal = QDir(baseDir).canonicalPath();
        QString candidate = QDir(QDir(baseDir).filePath(part)).canonicalPath();
        if (candidate.startsWith(baseReal))
            return candidate;
    } catch (...) {}
    return std::nullopt;
}

// ─────────────────────────────────────────────
//  SSRF / URL safety check
// ─────────────────────────────────────────────
bool isSafeRemoteUrl(const QString& url) {
    if (url.isEmpty() || url.length() > 4096) return false;
    for (QChar c : url)
        if (c.unicode() < 0x20 || c.unicode() == 0x7f) return false;

    QUrl u(url);
    if (!u.isValid()) return false;
    if (u.scheme() != u"http"_qs && u.scheme() != u"https"_qs) return false;
    if (u.host().isEmpty()) return false;
    if (!u.userName().isEmpty() || !u.password().isEmpty()) return false;

    // Resolve host – block private/loopback/reserved
    QHostInfo info = QHostInfo::fromName(u.host());
    if (info.addresses().isEmpty()) return false;

    for (const QHostAddress& addr : info.addresses()) {
        if (addr.isLoopback() || addr.isLinkLocal()) return false;
        // Check private ranges
        QString ip = addr.toString();
        QHostAddress a(addr);
        // IPv4 private ranges
        if (a.protocol() == QAbstractSocket::IPv4Protocol) {
            quint32 v = a.toIPv4Address();
            // 10.0.0.0/8
            if ((v >> 24) == 10) return false;
            // 172.16.0.0/12
            if ((v >> 20) == 0xAC1) return false;
            // 192.168.0.0/16
            if ((v >> 16) == 0xC0A8) return false;
            // 169.254.0.0/16 (link-local)
            if ((v >> 16) == 0xA9FE) return false;
            // 127.0.0.0/8 (loopback) – also caught by isLoopback
            if ((v >> 24) == 127) return false;
        }
        // IPv6 private/loopback already handled by isLoopback / isLinkLocal
    }
    return true;
}

// ─────────────────────────────────────────────
//  Formatting helpers
// ─────────────────────────────────────────────
QString fmtSize(double bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    while (bytes >= 1024.0 && i < 4) { bytes /= 1024.0; ++i; }
    return QStringLiteral("%1 %2").arg(bytes, 0, 'f', 1).arg(QLatin1StringView(units[i]));
}

QString fmtEta(int secs) {
    if (secs < 0) return u"?"_qs;
    int h = secs / 3600;
    int rem = secs % 3600;
    int m = rem / 60;
    int s = rem % 60;
    if (h > 0)
        return QStringLiteral("%1h%2m").arg(h).arg(m, 2, 10, QChar('0'));
    return QStringLiteral("%1m%2s").arg(m).arg(s, 2, 10, QChar('0'));
}

// ─────────────────────────────────────────────
//  Atomic write
// ─────────────────────────────────────────────
bool atomicWriteText(const QString& path, const QString& data) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(data.toUtf8());
    return f.commit();
}

// ─────────────────────────────────────────────
//  API token
// ─────────────────────────────────────────────
QString loadOrCreateApiToken() {
    const QString& tokenFile = appPaths().apiTokenFile;
    QFile f(tokenFile);
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString tok = QString::fromUtf8(f.readAll()).trimmed();
        f.close();
        static QRegularExpression valid(u"^[A-Za-z0-9_\\-]{32,128}$"_qs);
        if (valid.match(tok).hasMatch()) return tok;
    }

    // Generate 32 random bytes, base64url-encode
    QByteArray rnd(32, Qt::Uninitialized);
    QRandomGenerator::global()->fillRange(
        reinterpret_cast<quint32*>(rnd.data()),
        static_cast<qsizetype>(rnd.size() / sizeof(quint32)));
    QString tok = rnd.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    atomicWriteText(tokenFile, tok);
    return tok;
}

// ─────────────────────────────────────────────
//  yt-dlp / ffmpeg availability
// ─────────────────────────────────────────────
bool checkYtdlpAvailable() {
    const QStringList& cmd = appPaths().ytdlpCmd;
    if (cmd.isEmpty()) return false;
    QProcess p;
    p.setProgram(cmd.first());
    QStringList args = cmd.mid(1);
    args << u"--version"_qs;
    p.setArguments(args);
    p.start();
    return p.waitForFinished(5000) && p.exitCode() == 0;
}

QString getYtdlpVersion() {
    const QStringList& cmd = appPaths().ytdlpCmd;
    if (cmd.isEmpty()) return u"Not found"_qs;
    QProcess p;
    p.setProgram(cmd.first());
    QStringList args = cmd.mid(1);
    args << u"--version"_qs;
    p.setArguments(args);
    p.start();
    if (!p.waitForFinished(8000)) return u"Timeout"_qs;
    QString v = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    return v.isEmpty() ? u"Unknown"_qs : v;
}

QString getFfmpegVersion() {
    QString exe = appPaths().ffmpegDeployPath;
    if (!QFileInfo::exists(exe)) exe = u"ffmpeg"_qs;
    QProcess p;
    p.setProgram(exe);
    p.setArguments({ u"-version"_qs });
    p.start();
    if (!p.waitForFinished(8000)) return u"Not found"_qs;
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    if (out.isEmpty()) out = QString::fromUtf8(p.readAllStandardError());
    QString first = out.split('\n').first().trimmed();
    if (first.isEmpty()) return u"Unknown"_qs;
    static QRegularExpression re(u"(ffmpeg\\s+version\\s+[\\w.\\-]+)"_qs, QRegularExpression::CaseInsensitiveOption);
    auto m = re.match(first);
    return m.hasMatch() ? m.captured(1) : first.left(60);
}

// ─────────────────────────────────────────────
//  Unique destination
// ─────────────────────────────────────────────
QString uniqueDest(const QString& path) {
    if (!QFileInfo::exists(path)) return path;
    QString base = QFileInfo(path).completeBaseName();
    QString ext  = QFileInfo(path).suffix();
    QString dir  = QFileInfo(path).absolutePath();
    for (int i = 1; i <= 9999; ++i) {
        QString cand = QDir(dir).filePath(
            QStringLiteral("%1 (%2).%3").arg(base).arg(i).arg(ext));
        if (!QFileInfo::exists(cand)) return cand;
    }
    // Fallback: append random hex
    return QDir(QFileInfo(path).absolutePath()).filePath(
        base + '.' + QString::number(QRandomGenerator::global()->generate(), 16) + '.' + ext);
}
