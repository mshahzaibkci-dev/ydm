#pragma once
#include "downloaditem.h"
#include "scheduleitem.h"
#include <QList>
#include <QVariantMap>

namespace Persistence {
    // History (cross-session audit log)
    QList<QVariantMap> loadHistory();
    void saveHistory(const QList<QVariantMap>& entries);

    // Queue (resumed across sessions)
    QList<DownloadItem*> loadQueue();
    void saveQueue(const QList<DownloadItem*>& items);

    // Scheduler
    QList<ScheduledItem> loadSchedule();
    void saveSchedule(const QList<ScheduledItem>& items);
}
