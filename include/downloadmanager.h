#pragma once
#include "downloaditem.h"
#include <QObject>
#include <QList>
#include <QHash>
#include <QMutex>
#include <QThread>

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject* parent = nullptr);
    ~DownloadManager();

    void add(DownloadItem* item);
    void startItem(const QString& id);
    void pauseItem(const QString& id);
    void resumeItem(const QString& id);
    void cancelItem(const QString& id);
    void cancelAll();
    void clearFinished();

    bool urlInQueue(const QString& url) const;

    QList<DownloadItem*> queue;   // owned by manager

    // Called from persistence restore (no auto-launch)
    void restoreItem(DownloadItem* item);
    void tryStartNext();
    void persist();

signals:
    void itemAdded(DownloadItem* item);
    void itemUpdated(DownloadItem* item);
    void queueChanged();
    void logReceived(const QString& id, const QString& line);

private slots:
    void onProgress(const QString& id, double pct,
                    const QString& speed, const QString& eta,
                    const QString& size);
    void onStatus(const QString& id, DownloadStatus status);
    void onFilename(const QString& id, const QString& filename);
    void onLog(const QString& id, const QString& line);

private:
    DownloadItem* getItem(const QString& id) const;
    QThread*      getWorker(const QString& id) const;
    int           runningCount() const;
    void          launch(DownloadItem* item);
    void          resume(DownloadItem* item);

    QHash<QString, QThread*> m_workers;  // item id → worker thread
    mutable QMutex           m_mutex;
};
