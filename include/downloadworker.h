#pragma once
#include "downloaditem.h"
#include <QThread>
#include <QProcess>
#include <QMutex>
#include <atomic>
#include <deque>

// ─────────────────────────────────────────────
//  DownloadWorker  – runs yt-dlp in a QProcess
// ─────────────────────────────────────────────
class DownloadWorker : public QThread
{
    Q_OBJECT
public:
    explicit DownloadWorker(DownloadItem* item, QObject* parent = nullptr);

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
    void killProcessTree();
    void parseOutputLine(const QString& line);

    DownloadItem* m_item;
    QProcess*     m_proc = nullptr;
    QMutex        m_procMu;

    std::atomic<bool> m_cancelFlag{false};
    std::atomic<bool> m_pauseFlag{false};

    // Sliding window for speed smoothing (up to 8 samples)
    struct SpeedSample { qint64 ms; double pct; };
    std::deque<SpeedSample> m_speedSamples;
    static constexpr int SPEED_WINDOW = 8;
};
