#pragma once
#include "downloaditem.h"
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <atomic>
#include <deque>
#include <QPair>

// ─────────────────────────────────────────────
//  GenericFileWorker – HTTP chunked downloader
// ─────────────────────────────────────────────
class GenericFileWorker : public QThread
{
    Q_OBJECT
public:
    explicit GenericFileWorker(DownloadItem* item, QObject* parent = nullptr);

    void requestCancel();
    void requestPause();

signals:
    void progressUpdate(const QString& id, double pct,
                        const QString& speed, const QString& eta,
                        const QString& filesize);
    void logLine(const QString& id, const QString& line);
    void statusChanged(const QString& id, DownloadStatus status);
    void filenameFound(const QString& id, const QString& filename);

protected:
    void run() override;

private:
    // Returns the final (redirect-resolved) URL, or empty on SSRF failure.
    QString resolveUrl(const QString& url);
    // Returns filename from Content-Disposition or URL path.
    QString filenameFromResponse(const QNetworkReply* reply, const QString& url);

    DownloadItem*      m_item;
    std::atomic<bool>  m_cancelFlag{false};
    std::atomic<bool>  m_pauseFlag{false};

    static constexpr int   CHUNK_SIZE   = 256 * 1024;
    static constexpr int   MAX_REDIRECTS = 8;
    static constexpr int   MAX_RETRIES   = 3;
    static const char*     UA;
};
