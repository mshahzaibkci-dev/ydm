#pragma once
#include <QThread>

// Upgrades yt-dlp via pip in a background thread (Python: YtdlpUpdateWorker)
class YtdlpUpdateWorker : public QThread {
    Q_OBJECT
public:
    using QThread::QThread;
signals:
    void progressMsg(const QString& msg);
    void finishedOk(const QString& newVersion);
    void finishedErr(const QString& error);
protected:
    void run() override;
};
