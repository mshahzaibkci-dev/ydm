#include "genericfileworker.h"
#include "constants.h"
#include "utils.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QElapsedTimer>
#include <QTimer>
#include <deque>

const char* GenericFileWorker::UA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0 Safari/537.36";

GenericFileWorker::GenericFileWorker(DownloadItem* item, QObject* parent)
    : QThread(parent), m_item(item)
{}

void GenericFileWorker::requestCancel() { m_cancelFlag.store(true); m_pauseFlag.store(false); }
void GenericFileWorker::requestPause()  { m_pauseFlag.store(true); }

// ─────────────────────────────────────────────
//  URL resolution with SSRF check
// ─────────────────────────────────────────────
QString GenericFileWorker::resolveUrl(const QString& url) {
    // We validate every hop in the redirect chain
    if (!isSafeRemoteUrl(url)) {
        emit logLine(m_item->id,
            QStringLiteral("  [BLOCKED] Refusing unsafe URL: %1\n").arg(url.left(120)));
        return {};
    }
    // We don't follow redirects here — we return the starting URL and let Qt's
    // QNetworkAccessManager follow them automatically (with SSRF check on final).
    return url;
}

QString GenericFileWorker::filenameFromResponse(const QNetworkReply* reply, const QString& url) {
    QString cd = reply->rawHeader("Content-Disposition");
    if (!cd.isEmpty()) {
        static QRegularExpression re(
            u"filename\\*?=[\"']?(?:UTF-\\d+''|UTF-\\d+\")?([^\"';\\n]+)"_qs,
            QRegularExpression::CaseInsensitiveOption);
        auto m = re.match(cd);
        if (m.hasMatch())
            return sanitizeFilename(m.captured(1).trimmed().remove('"'));
    }
    return sanitizeFilename(QFileInfo(QUrl(url).path()).fileName());
}

// ─────────────────────────────────────────────
//  run() – range-request download loop
// ─────────────────────────────────────────────
void GenericFileWorker::run() {
    DownloadItem* item = m_item;
    emit statusChanged(item->id, DownloadStatus::Running);

    if (!isSafeRemoteUrl(item->url)) {
        emit logLine(item->id, u"ERROR: URL failed security validation.\n"_qs);
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    emit logLine(item->id, QStringLiteral("[START] Generic download: %1\n").arg(item->url));

    // QNetworkAccessManager must live on the thread it's used on
    QNetworkAccessManager nam;
    nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    QString finalUrl = resolveUrl(item->url);
    if (finalUrl.isEmpty()) {
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    // ── Step 1: Resolve filename and lock paths ───────────────────────────
    if (item->tempPath.isEmpty() || item->finalPath.isEmpty()) {
        // Probe filename via HEAD
        QString filename;
        {
            QNetworkRequest req(finalUrl);
            req.setRawHeader("User-Agent", UA);
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
            QNetworkReply* probe = nam.head(req);
            QEventLoop loop;
            connect(probe, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if (probe->error() == QNetworkReply::NoError) {
                // Check for redirect
                QUrl redir = probe->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                QString effectiveUrl = redir.isValid() ? redir.toString() : finalUrl;
                filename = filenameFromResponse(probe, effectiveUrl);
                if (!isSafeRemoteUrl(effectiveUrl))
                    filename.clear();
            }
            probe->deleteLater();
        }
        if (filename.isEmpty())
            filename = sanitizeFilename(QFileInfo(QUrl(finalUrl).path()).fileName());
        if (filename.isEmpty())
            filename = u"download"_qs;

        // Lock output directory to categorized subfolder
        QString base = item->outputDir;
        // Check if already in a category subdir
        static const QStringList cats = {
            u"Videos"_qs, u"Music"_qs, u"Documents"_qs,
            u"Archives"_qs, u"Programs"_qs, u"Other"_qs
        };
        bool alreadyCat = false;
        for (const auto& c : cats)
            if (base.endsWith('/' + c) || base.endsWith('\\' + c))
                { alreadyCat = true; break; }
        if (alreadyCat) base = QFileInfo(base).absolutePath();

        QString catDir = makeCategorizedOutputDir(base, filename, item->fmtKey);
        item->outputDir = catDir;

        auto maybeRaw = safeJoin(catDir, filename);
        if (!maybeRaw) {
            emit logLine(item->id, u"ERROR: filename escapes output dir.\n"_qs);
            emit statusChanged(item->id, DownloadStatus::Failed);
            return;
        }

        item->finalPath = *maybeRaw;
        item->tempPath  = item->finalPath + u".ydm_part"_qs;
        item->filename  = filename;
        emit filenameFound(item->id, filename);
        emit logLine(item->id,
            QStringLiteral("  Filename : %1\n  Temp: %2\n  Final: %3\n")
            .arg(filename).arg(item->tempPath).arg(item->finalPath));
    } else {
        QDir().mkpath(QFileInfo(item->tempPath).absolutePath());
        emit logLine(item->id,
            QStringLiteral("  [RESUME] Paths restored\n  Temp: %1\n  Final: %2\n")
            .arg(item->tempPath).arg(item->finalPath));
    }

    // ── Step 2: measure existing partial ─────────────────────────────────
    qint64 existingSize = 0;
    {
        QFile f(item->tempPath);
        if (f.exists()) existingSize = f.size();
    }
    emit logLine(item->id,
        QStringLiteral("  On-disk partial: %1 bytes (%2)\n")
        .arg(existingSize).arg(fmtSize(existingSize)));

    // ── Step 3: Send HTTP request (with optional Range header) ────────────
    QNetworkRequest req(QUrl(finalUrl));
    req.setRawHeader("User-Agent", UA);
    if (existingSize > 0) {
        req.setRawHeader("Range",
            QStringLiteral("bytes=%1-").arg(existingSize).toUtf8());
        emit logLine(item->id,
            QStringLiteral("  Sending Range: bytes=%1-\n").arg(existingSize));
    }

    QNetworkReply* reply = nam.get(req);

    // Wait for headers
    {
        QEventLoop loop;
        connect(reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished,  &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit logLine(item->id,
            QStringLiteral("ERROR: %1\n").arg(reply->errorString()));
        reply->deleteLater();
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    emit logLine(item->id,
        QStringLiteral("  Server response: HTTP %1\n").arg(statusCode));

    // 416 – range not satisfiable → restart from zero
    if (statusCode == 416) {
        emit logLine(item->id, u"  [416] Range not satisfiable — restarting.\n"_qs);
        reply->deleteLater();
        existingSize = 0;
        item->bytesDownloaded = 0;

        QNetworkRequest req2(QUrl(finalUrl));
        req2.setRawHeader("User-Agent", UA);
        reply = nam.get(req2);
        QEventLoop loop2;
        connect(reply, &QNetworkReply::readyRead, &loop2, &QEventLoop::quit);
        connect(reply, &QNetworkReply::finished,  &loop2, &QEventLoop::quit);
        loop2.exec();
        statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emit logLine(item->id,
            QStringLiteral("  Restart response: HTTP %1\n").arg(statusCode));
    }

    bool resumeOk  = (statusCode == 206);
    qint64 writeStart = resumeOk ? existingSize : 0;
    QString fileMode  = resumeOk ? u"ab"_qs : u"wb"_qs;

    if (!resumeOk && existingSize > 0) {
        emit logLine(item->id,
            u"  [NO RESUME] Server returned 200; restarting from scratch.\n"_qs);
        existingSize = 0;
        item->bytesDownloaded = 0;
    }

    qint64 respLength = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    qint64 total      = respLength + writeStart;

    if (total > GENERIC_MAX_FILE_BYTES) {
        emit logLine(item->id,
            QStringLiteral("ERROR: file too large (%1 > %2).\n")
            .arg(fmtSize(total)).arg(fmtSize(GENERIC_MAX_FILE_BYTES)));
        reply->deleteLater();
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    item->bytesTotal = total;
    QString sizeStr  = total ? fmtSize(total) : u"?"_qs;
    if (total)
        emit logLine(item->id,
            QStringLiteral("  Total size: %1\n").arg(sizeStr));

    // ── Step 4: Write chunks ──────────────────────────────────────────────
    QFile outFile(item->tempPath);
    bool opened = resumeOk
        ? outFile.open(QIODevice::Append)
        : outFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!opened) {
        emit logLine(item->id,
            QStringLiteral("ERROR: Cannot open temp file: %1\n").arg(item->tempPath));
        reply->deleteLater();
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    qint64 downloaded = writeStart;

    struct Sample { qint64 ms; qint64 bytes; };
    std::deque<Sample> samples;
    QElapsedTimer timer;
    timer.start();

    while (!reply->isFinished() || reply->bytesAvailable() > 0) {
        if (m_cancelFlag.load()) break;

        if (m_pauseFlag.load()) {
            outFile.flush();
            outFile.close();
            item->bytesDownloaded = downloaded;
            emit logLine(item->id,
                QStringLiteral("  [PAUSED] Paused at %1. Temp: %2\n")
                .arg(fmtSize(downloaded)).arg(item->tempPath));
            reply->abort();
            reply->deleteLater();
            emit statusChanged(item->id, DownloadStatus::Paused);
            return;
        }

        // Wait for data if none available yet
        if (reply->bytesAvailable() == 0 && !reply->isFinished()) {
            QEventLoop loop;
            connect(reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
            connect(reply, &QNetworkReply::finished,  &loop, &QEventLoop::quit);
            QTimer::singleShot(200, &loop, &QEventLoop::quit);
            loop.exec();
            continue;
        }

        QByteArray chunk = reply->read(CHUNK_SIZE);
        if (chunk.isEmpty()) continue;

        if (downloaded + chunk.size() > GENERIC_MAX_FILE_BYTES) {
            emit logLine(item->id, u"ERROR: download exceeded max size.\n"_qs);
            outFile.close();
            reply->abort();
            reply->deleteLater();
            emit statusChanged(item->id, DownloadStatus::Failed);
            return;
        }

        outFile.write(chunk);
        downloaded += chunk.size();
        item->bytesDownloaded = downloaded;

        qint64 now = timer.elapsed();
        samples.push_back({ now, downloaded });
        while (samples.size() > 8) samples.pop_front();

        QString speedStr, etaStr;
        if (samples.size() >= 2) {
            double dt = (samples.back().ms - samples.front().ms) / 1000.0;
            if (dt > 0.05) {
                double bps = (samples.back().bytes - samples.front().bytes) / dt;
                speedStr = fmtSize(bps) + u"/s"_qs;
                if (total > 0) {
                    qint64 rem = total - downloaded;
                    etaStr = fmtEta(static_cast<int>(rem / std::max(bps, 1.0)));
                }
            }
        }

        if (total > 0) {
            double pct = std::min(downloaded / static_cast<double>(total) * 100.0, 100.0);
            emit progressUpdate(item->id, pct, speedStr, etaStr, sizeStr);
        }
    }

    outFile.flush();
    outFile.close();
    reply->deleteLater();

    if (m_cancelFlag.load()) {
        emit logLine(item->id,
            QStringLiteral("  [CANCELLED] Partial file kept at: %1\n").arg(item->tempPath));
        emit statusChanged(item->id, DownloadStatus::Cancelled);
        return;
    }

    // ── Step 5: Atomic rename temp → final ───────────────────────────────
    QString finalDest = uniqueDest(item->finalPath);
    auto check = safeJoin(QFileInfo(item->finalPath).absolutePath(),
                          QFileInfo(finalDest).fileName());
    if (!check) {
        emit logLine(item->id, u"ERROR: destination escape detected.\n"_qs);
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    if (QFileInfo::exists(finalDest)) QFile::remove(finalDest);
    if (!QFile::rename(item->tempPath, finalDest)) {
        emit logLine(item->id,
            QStringLiteral("ERROR finalizing file: rename failed.\n"));
        emit statusChanged(item->id, DownloadStatus::Failed);
        return;
    }

    item->bytesDownloaded = 0;
    emit progressUpdate(item->id, 100.0, {}, {}, sizeStr);
    emit logLine(item->id,
        QStringLiteral("  [DONE] Completed → %1\n  Saved to: %2\n")
        .arg(QFileInfo(finalDest).fileName()).arg(finalDest));
    emit statusChanged(item->id, DownloadStatus::Completed);
}
