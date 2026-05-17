#include "downloadmanager.h"
#include "downloadworker.h"
#include "genericfileworker.h"
#include "persistence.h"
#include "utils.h"
#include "constants.h"
#include <QMutexLocker>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
{}

DownloadManager::~DownloadManager() {
    for (auto* item : std::as_const(queue)) delete item;
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
void DownloadManager::add(DownloadItem* item) {
    queue.append(item);
    emit itemAdded(item);
    tryStartNext();
    persist();
}

void DownloadManager::restoreItem(DownloadItem* item) {
    queue.append(item);
}

void DownloadManager::startItem(const QString& id) {
    DownloadItem* item = getItem(id);
    if (!item) return;
    if (item->status == DownloadStatus::Paused)
        resume(item);
    else if (item->status == DownloadStatus::Queued
          || item->status == DownloadStatus::Failed) {
        item->logs.clear();
        safeTransition(item->status, DownloadStatus::Queued);
        launch(item);
    }
}

void DownloadManager::pauseItem(const QString& id) {
    DownloadItem* item = getItem(id);
    if (!item || item->status != DownloadStatus::Running) return;
    QThread* w = getWorker(id);
    if (!w) return;

    if (auto* dw = qobject_cast<DownloadWorker*>(w))
        dw->requestPause();
    else if (auto* gw = qobject_cast<GenericFileWorker*>(w))
        gw->requestPause();

    item->speed.clear();
    item->eta.clear();
    emit itemUpdated(item);
}

void DownloadManager::resumeItem(const QString& id) {
    DownloadItem* item = getItem(id);
    if (!item || item->status != DownloadStatus::Paused) return;
    resume(item);
}

void DownloadManager::cancelItem(const QString& id) {
    DownloadItem* item = getItem(id);
    if (!item) return;

    if (item->status == DownloadStatus::Queued) {
        safeTransition(item->status, DownloadStatus::Cancelled);
        emit itemUpdated(item);
        tryStartNext();
        persist();
        return;
    }
    if (item->status == DownloadStatus::Paused) {
        safeTransition(item->status, DownloadStatus::Cancelling);
        safeTransition(item->status, DownloadStatus::Cancelled);
        emit itemUpdated(item);
        persist();
        return;
    }

    QThread* w = getWorker(id);
    if (w) {
        safeTransition(item->status, DownloadStatus::Cancelling);
        emit itemUpdated(item);
        if (auto* dw = qobject_cast<DownloadWorker*>(w))
            dw->requestCancel();
        else if (auto* gw = qobject_cast<GenericFileWorker*>(w))
            gw->requestCancel();
    }
}

void DownloadManager::cancelAll() {
    for (auto* item : std::as_const(queue))
        if (!isTerminal(item->status))
            cancelItem(item->id);
}

void DownloadManager::clearFinished() {
    QList<DownloadItem*> keep;
    for (auto* item : std::as_const(queue)) {
        if (isTerminal(item->status))
            delete item;
        else
            keep.append(item);
    }
    queue = keep;
    emit queueChanged();
    persist();
}

bool DownloadManager::urlInQueue(const QString& url) const {
    for (const auto* item : std::as_const(queue))
        if (item->url == url && !isTerminal(item->status))
            return true;
    return false;
}

// ─────────────────────────────────────────────
//  Private helpers
// ─────────────────────────────────────────────
DownloadItem* DownloadManager::getItem(const QString& id) const {
    for (auto* item : std::as_const(queue))
        if (item->id == id) return item;
    return nullptr;
}

QThread* DownloadManager::getWorker(const QString& id) const {
    QMutexLocker lk(&m_mutex);
    return m_workers.value(id, nullptr);
}

int DownloadManager::runningCount() const {
    int n = 0;
    for (const auto* item : std::as_const(queue))
        if (item->status == DownloadStatus::Running
         || item->status == DownloadStatus::Starting
         || item->status == DownloadStatus::Cancelling)
            ++n;
    return n;
}

void DownloadManager::tryStartNext() {
    while (runningCount() < MAX_CONCURRENT) {
        DownloadItem* next = nullptr;
        for (auto* item : std::as_const(queue))
            if (item->status == DownloadStatus::Queued) { next = item; break; }
        if (!next) break;
        launch(next);
    }
}

void DownloadManager::launch(DownloadItem* item) {
    {
        QMutexLocker lk(&m_mutex);
        if (m_workers.contains(item->id)) return;
    }
    safeTransition(item->status, DownloadStatus::Starting);
    emit itemUpdated(item);

    QThread* worker = nullptr;
    if (shouldUseYtdlp(item->url)) {
        auto* dw = new DownloadWorker(item);
        connect(dw, &DownloadWorker::progressUpdate, this, &DownloadManager::onProgress);
        connect(dw, &DownloadWorker::statusChanged,  this, &DownloadManager::onStatus);
        connect(dw, &DownloadWorker::filenameFound,  this, &DownloadManager::onFilename);
        connect(dw, &DownloadWorker::logLine,        this, &DownloadManager::onLog);
        worker = dw;
    } else {
        auto* gw = new GenericFileWorker(item);
        connect(gw, &GenericFileWorker::progressUpdate, this, &DownloadManager::onProgress);
        connect(gw, &GenericFileWorker::statusChanged,  this, &DownloadManager::onStatus);
        connect(gw, &GenericFileWorker::filenameFound,  this, &DownloadManager::onFilename);
        connect(gw, &GenericFileWorker::logLine,        this, &DownloadManager::onLog);
        worker = gw;
    }

    // Clean up worker after it finishes
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);

    {
        QMutexLocker lk(&m_mutex);
        m_workers.insert(item->id, worker);
    }
    worker->start();
}

void DownloadManager::resume(DownloadItem* item) {
    if (item->status != DownloadStatus::Paused) return;
    item->status = DownloadStatus::Queued;
    item->logs.append(u"[RESUME] Resuming...\n"_qs);
    launch(item);
}

// ─────────────────────────────────────────────
//  Slots from workers
// ─────────────────────────────────────────────
void DownloadManager::onProgress(const QString& id, double pct,
                                  const QString& speed, const QString& eta,
                                  const QString& size) {
    DownloadItem* item = getItem(id);
    if (item && item->status == DownloadStatus::Running) {
        item->progress = pct;
        item->speed    = speed;
        item->eta      = eta;
        if (!size.isEmpty()) item->filesize = size;
        emit itemUpdated(item);
    }
}

void DownloadManager::onStatus(const QString& id, DownloadStatus status) {
    DownloadItem* item = getItem(id);
    if (item) {
        // CANCELLING + FAILED → CANCELLED
        if (item->status == DownloadStatus::Cancelling
                && status == DownloadStatus::Failed)
            status = DownloadStatus::Cancelled;

        bool applied = safeTransition(item->status, status);
        if (!applied && isTerminal(status))
            item->status = status;

        if (status == DownloadStatus::Completed) {
            item->progress = 100.0;
            item->speed.clear();
            item->eta.clear();
        } else if (status == DownloadStatus::Paused) {
            item->speed.clear();
            item->eta.clear();
        }
        emit itemUpdated(item);
    }

    if (isTerminal(status) || status == DownloadStatus::Paused) {
        QMutexLocker lk(&m_mutex);
        m_workers.remove(id);
    }
    if (isTerminal(status))
        tryStartNext();

    persist();
}

void DownloadManager::onFilename(const QString& id, const QString& filename) {
    DownloadItem* item = getItem(id);
    if (item && !filename.isEmpty()) {
        item->filename = sanitizeFilename(filename);
        emit itemUpdated(item);
        persist();
    }
}

void DownloadManager::onLog(const QString& id, const QString& line) {
    DownloadItem* item = getItem(id);
    if (item) item->logs.append(line);
    emit logReceived(id, line);
}

void DownloadManager::persist() {
    Persistence::saveQueue(queue);
}
