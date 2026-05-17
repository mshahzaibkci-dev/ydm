#pragma once
#include "constants.h"
#include <QString>
#include <QStringList>
#include <QDateTime>

// ─────────────────────────────────────────────
//  DownloadItem  – data model (POD-ish struct)
// ─────────────────────────────────────────────
struct DownloadItem {
    QString         id;
    QString         url;
    QString         outputDir;
    QString         fmtKey;
    QString         fmtValue;
    DownloadStatus  status     = DownloadStatus::Queued;
    double          progress   = 0.0;
    QString         speed;
    QString         eta;
    QString         filesize;
    QString         filename;
    QString         addedAt;
    QString         errorMsg;
    QStringList     logs;
    bool            isDirect   = false;
    qint64          bytesDownloaded = 0;
    qint64          bytesTotal      = 0;
    QString         displayFmt;
    // GenericFileWorker path lock
    QString         tempPath;
    QString         finalPath;
    // yt-dlp playlist mode
    bool            isPlaylist = false;

    // Constructs a new item with a UUID-derived id
    static DownloadItem create(const QString& url,
                               const QString& outputDir,
                               const QString& fmtKey);
};
