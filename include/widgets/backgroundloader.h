#pragma once
#include "downloaditem.h"
#include <QThread>
#include <QVariantMap>
#include <QList>

// Background loader matching Python _BackgroundLoader
class BackgroundLoader : public QThread {
    Q_OBJECT
public:
    explicit BackgroundLoader(QObject* parent = nullptr);

signals:
    void historyReady(const QList<QVariantMap>& entries);
    void queueReady(const QList<DownloadItem*>& items);

protected:
    void run() override;
};
