#include "ffmpegupdateworker.h"
#include "constants.h"
#include "utils.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QProcess>

const char* FfmpegUpdateWorker::RELEASES_URL =
    "https://github.com/yt-dlp/FFmpeg-Builds/releases/download/latest"
    "/ffmpeg-master-latest-win64-gpl.zip";

void FfmpegUpdateWorker::run() {
    emit progressMsg(u"Connecting to yt-dlp FFmpeg builds…"_qs);

    const QString& deployPath = appPaths().ffmpegDeployPath;
    QString deployDir = QFileInfo(deployPath).absolutePath();
    QDir().mkpath(deployDir);

    // ── 1. Download zip to a temp file ────────────────────────────────────
    QTemporaryFile tmpZip(deployDir + u"/.ffmpeg_update_XXXXXX.zip"_qs);
    if (!tmpZip.open()) {
        emit finishedErr(u"Cannot create temp file in app directory."_qs);
        return;
    }
    tmpZip.setAutoRemove(true);

    QNetworkAccessManager nam;
    QNetworkRequest req(QUrl(QLatin1StringView(RELEASES_URL)));
    req.setRawHeader("User-Agent", "RAINAX-Updater/2.0");

    QNetworkReply* reply = nam.get(req);
    qint64 downloaded = 0;
    qint64 total = 0;

    QEventLoop loop;
    connect(reply, &QNetworkReply::downloadProgress,
            [&](qint64 recv, qint64 tot) {
        downloaded = recv;
        total = tot;
        if (tot > 0) {
            int pct = int(recv * 100 / tot);
            emit progressMsg(QStringLiteral("Downloading ffmpeg… %1%  (%2 / %3)")
                             .arg(pct)
                             .arg(fmtSize(recv))
                             .arg(fmtSize(tot)));
        }
        if (recv > MAX_BYTES) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer::singleShot(300000, &loop, &QEventLoop::quit);  // 5 min timeout
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        reply->deleteLater();
        emit finishedErr(QStringLiteral("Download failed:\n%1").arg(err));
        return;
    }

    QByteArray zipData = reply->readAll();
    reply->deleteLater();

    if (zipData.size() > MAX_BYTES) {
        emit finishedErr(u"Download exceeds safety size cap."_qs);
        return;
    }

    tmpZip.write(zipData);
    tmpZip.flush();

    // ── 2. Extract ffmpeg.exe from zip using 'unzip' or PowerShell ───────
    emit progressMsg(u"Extracting ffmpeg.exe…"_qs);

    QString tmpZipPath = tmpZip.fileName();

    // Try PowerShell expand-archive (Windows only)
    QTemporaryFile tmpExe(deployDir + u"/.ffmpeg_new_XXXXXX.exe"_qs);
    if (!tmpExe.open()) {
        emit finishedErr(u"Cannot create temp exe file."_qs);
        return;
    }
    tmpExe.setAutoRemove(true);
    QString tmpExePath = tmpExe.fileName();
    tmpExe.close();

    // Use PowerShell to extract the exe
    QString psScript = QStringLiteral(
        "$zip = [System.IO.Compression.ZipFile]::OpenRead('%1');"
        "$entry = $zip.Entries | Where-Object { $_.FullName -match 'bin/ffmpeg\\.exe$' } | Select -First 1;"
        "if ($entry) { [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, '%2', $true); Write-Output 'ok' }"
        "else { Write-Error 'not found' }"
        "$zip.Dispose()"
    ).arg(tmpZipPath.replace('/', '\\')).arg(tmpExePath.replace('/', '\\'));

    QProcess ps;
    ps.setProgram(u"powershell.exe"_qs);
    ps.setArguments({
        u"-NoProfile"_qs, u"-NonInteractive"_qs,
        u"-Command"_qs, QStringLiteral(
            "Add-Type -Assembly 'System.IO.Compression.FileSystem'; %1").arg(psScript)
    });
    ps.start();
    if (!ps.waitForFinished(60000)) {
        ps.kill();
        emit finishedErr(u"Extraction timed out."_qs);
        return;
    }
    if (ps.exitCode() != 0 || !QFileInfo::exists(tmpExePath)) {
        emit finishedErr(QStringLiteral("Extraction failed:\n%1")
                         .arg(QString::fromUtf8(ps.readAllStandardError()).trimmed()));
        return;
    }

    // ── 3. Atomically replace old ffmpeg.exe ─────────────────────────────
    emit progressMsg(u"Installing ffmpeg.exe…"_qs);

    if (QFileInfo::exists(deployPath)) QFile::remove(deployPath);
    if (!QFile::rename(tmpExePath, deployPath)) {
        emit finishedErr(QStringLiteral(
            "Could not replace ffmpeg.exe.\nTry running RAINAX as Administrator."));
        return;
    }
    tmpExe.setAutoRemove(false);  // already moved

    emit finishedOk(getFfmpegVersion());
}
