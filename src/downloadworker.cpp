#include "downloadworker.h"
#include "constants.h"
#include "utils.h"
#include <QRegularExpression>
#include <QMutexLocker>
#include <QDir>
#include <QElapsedTimer>
#include <QDateTime>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

DownloadWorker::DownloadWorker(DownloadItem* item, QObject* parent)
    : QThread(parent), m_item(item)
{}

void DownloadWorker::requestCancel() {
    m_cancelFlag.store(true);
    m_pauseFlag.store(false);
    killProcessTree();
}

void DownloadWorker::requestPause() {
    m_pauseFlag.store(true);
    killProcessTree();
}

void DownloadWorker::killProcessTree() {
    QMutexLocker lk(&m_procMu);
    if (!m_proc) return;

#ifdef Q_OS_WIN
    // Kill process tree on Windows via taskkill
    qint64 pid = m_proc->processId();
    if (pid) {
        QProcess::execute(
            u"taskkill"_qs,
            { u"/F"_qs, u"/T"_qs, u"/PID"_qs, QString::number(pid) }
        );
    }
#else
    m_proc->kill();
#endif
}

// ─────────────────────────────────────────────
//  run() – the actual yt-dlp subprocess
// ─────────────────────────────────────────────
void DownloadWorker::run() {
    DownloadItem* item = m_item;
    emit statusChanged(item->id, DownloadStatus::Running);

    // Build yt-dlp command
    const QStringList& base = appPaths().ytdlpCmd;
    QStringList args = base.mid(1); // skip program name

    // Categorized output directory
    QString catDir = makeCategorizedOutputDir(item->outputDir, {}, item->fmtKey);
    item->outputDir = catDir;
    QDir().mkpath(catDir);

    QString outTemplate = QDir(catDir).filePath(u"%(title)s.%(ext)s"_qs);

    args << u"--newline"_qs
         << u"--progress"_qs
         << u"-o"_qs << outTemplate
         << u"-f"_qs << item->fmtValue;

    // Playlist handling
    if (!item->isPlaylist)
        args << u"--no-playlist"_qs;

    // ffmpeg location
    const QString& ffmpegExe = appPaths().ffmpegDeployPath;
    if (QFileInfo::exists(ffmpegExe))
        args << u"--ffmpeg-location"_qs << QFileInfo(ffmpegExe).absolutePath();

    args << item->url;

    emit logLine(item->id,
        QStringLiteral("[START] yt-dlp %1\n  Format: %2\n  Out: %3\n")
        .arg(item->url).arg(item->fmtKey).arg(catDir));

    QProcess proc;
    proc.setProgram(base.first());
    proc.setArguments(args);
    proc.setProcessChannelMode(QProcess::MergedChannels);

    {
        QMutexLocker lk(&m_procMu);
        m_proc = &proc;
    }

    proc.start();
    if (!proc.waitForStarted(10000)) {
        emit logLine(item->id, u"ERROR: Failed to start yt-dlp process.\n"_qs);
        emit statusChanged(item->id, DownloadStatus::Failed);
        QMutexLocker lk(&m_procMu);
        m_proc = nullptr;
        return;
    }

    // Read output line-by-line
    while (proc.state() != QProcess::NotRunning) {
        if (m_cancelFlag.load() || m_pauseFlag.load()) {
            killProcessTree();
            break;
        }

        proc.waitForReadyRead(100);
        while (proc.canReadLine()) {
            QString line = QString::fromUtf8(proc.readLine());
            emit logLine(item->id, line);
            parseOutputLine(line);
        }
    }

    // Drain remaining output
    while (proc.canReadLine()) {
        QString line = QString::fromUtf8(proc.readLine());
        emit logLine(item->id, line);
        parseOutputLine(line);
    }

    proc.waitForFinished(3000);
    int exitCode = proc.exitCode();

    {
        QMutexLocker lk(&m_procMu);
        m_proc = nullptr;
    }

    if (m_cancelFlag.load()) {
        emit statusChanged(item->id, DownloadStatus::Cancelled);
    } else if (m_pauseFlag.load()) {
        emit statusChanged(item->id, DownloadStatus::Paused);
    } else if (exitCode == 0) {
        emit progressUpdate(item->id, 100.0, {}, {}, item->filesize);
        emit statusChanged(item->id, DownloadStatus::Completed);
    } else {
        emit logLine(item->id,
            QStringLiteral("ERROR: yt-dlp exited with code %1\n").arg(exitCode));
        emit statusChanged(item->id, DownloadStatus::Failed);
    }
}

// ─────────────────────────────────────────────
//  Progress / filename parsing
// ─────────────────────────────────────────────
void DownloadWorker::parseOutputLine(const QString& line) {
    DownloadItem* item = m_item;

    // [download] XX.X% of ~YYY.YYMB at ZZZ.ZKiB/s ETA HH:MM
    static const QRegularExpression reProgress(
        uR"(\[download\]\s+([\d.]+)%\s+of(?:\s+~)?\s*([\d.]+\s*\S+)"
        R"((?:\s+at\s+([\d.]+\s*\S+/s))?(?:\s+ETA\s+(\S+))?)"_qs);

    // [download] Destination: /path/to/file.ext
    static const QRegularExpression reDest(
        u"\\[download\\] Destination:\\s+(.+)"_qs);

    // [Merger] Merging formats into "file.mp4"
    static const QRegularExpression reMerge(
        u"\\[Merger\\].*?\"(.+?)\""_qs);

    auto mProgress = reProgress.match(line);
    if (mProgress.hasMatch()) {
        double pct   = mProgress.captured(1).toDouble();
        QString size = mProgress.captured(2).trimmed();
        QString spd  = mProgress.captured(3).trimmed();
        QString eta  = mProgress.captured(4).trimmed();

        if (item->status != DownloadStatus::Running)
            emit statusChanged(item->id, DownloadStatus::Running);

        emit progressUpdate(item->id, pct, spd, eta, size);
        return;
    }

    auto mDest = reDest.match(line);
    if (mDest.hasMatch()) {
        QString path = mDest.captured(1).trimmed();
        QString fn   = QFileInfo(path).fileName();
        if (!fn.isEmpty())
            emit filenameFound(item->id, fn);
        return;
    }

    auto mMerge = reMerge.match(line);
    if (mMerge.hasMatch()) {
        QString path = mMerge.captured(1).trimmed();
        QString fn   = QFileInfo(path).fileName();
        if (!fn.isEmpty())
            emit filenameFound(item->id, fn);
    }
}
