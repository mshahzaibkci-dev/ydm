#include "persistence.h"
#include "constants.h"
#include "utils.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

namespace Persistence {

// ─────────────────────────────────────────────
//  History
// ─────────────────────────────────────────────
QList<QVariantMap> loadHistory() {
    const QString& path = appPaths().historyFile;
    if (!QFileInfo::exists(path)) return {};
    if (QFileInfo(path).size() > HISTORY_MAX_BYTES) return {};

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return {};

    QList<QVariantMap> out;
    for (const auto& v : doc.array()) {
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();
        QVariantMap e;
        e[u"time"_qs]     = o[u"time"_qs].toString().left(32);
        e[u"url"_qs]      = o[u"url"_qs].toString().left(4096);
        e[u"filename"_qs] = o[u"filename"_qs].toString().left(512);
        e[u"status"_qs]   = o[u"status"_qs].toString().left(32);
        out.append(e);
        if (out.size() >= HISTORY_MAX_ENTRIES) break;
    }
    return out;
}

void saveHistory(const QList<QVariantMap>& entries) {
    QJsonArray arr;
    qsizetype start = std::max(qsizetype(0), entries.size() - HISTORY_MAX_ENTRIES);
    for (int i = start; i < entries.size(); ++i) {
        const auto& e = entries[i];
        QJsonObject o;
        o[u"time"_qs]     = e.value(u"time"_qs).toString();
        o[u"url"_qs]      = e.value(u"url"_qs).toString();
        o[u"filename"_qs] = e.value(u"filename"_qs).toString();
        o[u"status"_qs]   = e.value(u"status"_qs).toString();
        arr.append(o);
    }
    atomicWriteText(appPaths().historyFile,
                    QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

// ─────────────────────────────────────────────
//  Queue serialization helpers
// ─────────────────────────────────────────────
static QJsonObject itemToJson(const DownloadItem* item) {
    QJsonObject o;
    o[u"id"_qs]               = item->id;
    o[u"url"_qs]              = item->url;
    o[u"output_dir"_qs]       = item->outputDir;
    o[u"fmt_key"_qs]          = item->fmtKey;
    o[u"display_fmt"_qs]      = item->displayFmt;
    o[u"status"_qs]           = statusToString(item->status);
    o[u"progress"_qs]         = item->progress;
    o[u"filename"_qs]         = item->filename;
    o[u"filesize"_qs]         = item->filesize;
    o[u"added_at"_qs]         = item->addedAt;
    o[u"error_msg"_qs]        = item->errorMsg;
    o[u"is_direct"_qs]        = item->isDirect;
    o[u"bytes_downloaded"_qs] = item->bytesDownloaded;
    o[u"bytes_total"_qs]      = item->bytesTotal;
    o[u"temp_path"_qs]        = item->tempPath;
    o[u"final_path"_qs]       = item->finalPath;
    o[u"is_playlist"_qs]      = item->isPlaylist;
    return o;
}

static DownloadItem* itemFromJson(const QJsonObject& d) {
    QString url       = d[u"url"_qs].toString().trimmed();
    QString outputDir = d[u"output_dir"_qs].toString().trimmed();
    if (url.isEmpty() || outputDir.isEmpty()) return nullptr;

    auto* item = new DownloadItem;
    item->id         = d[u"id"_qs].toString().left(16);
    item->url        = url;
    item->outputDir  = outputDir;

    QString fmtKey   = d[u"fmt_key"_qs].toString();
    item->fmtKey     = isValidFormatKey(fmtKey) ? fmtKey : u"Best Quality (auto)"_qs;
    item->fmtValue   = formatValueForKey(item->fmtKey);
    item->isDirect   = d[u"is_direct"_qs].toBool(isDirectFileUrl(url));

    QString rawDisplayFmt = d[u"display_fmt"_qs].toString();
    if (!rawDisplayFmt.isEmpty())
        item->displayFmt = rawDisplayFmt.left(128);
    else if (shouldUseYtdlp(url))
        item->displayFmt = item->fmtKey;
    else
        item->displayFmt = getGenericFileLabel(url, d[u"filename"_qs].toString());

    // Crash-interrupt recovery
    static const QSet<DownloadStatus> crashStates = {
        DownloadStatus::Running, DownloadStatus::Starting, DownloadStatus::Cancelling
    };
    DownloadStatus restored = statusFromString(d[u"status"_qs].toString());
    if (crashStates.contains(restored)) restored = DownloadStatus::Queued;

    item->status    = restored;
    item->progress  = crashStates.contains(restored) ? 0.0 : d[u"progress"_qs].toDouble();
    item->speed     = {};
    item->eta       = {};
    item->filename  = d[u"filename"_qs].toString().left(512);
    item->filesize  = d[u"filesize"_qs].toString().left(64);
    item->addedAt   = d[u"added_at"_qs].toString().left(32);
    if (item->addedAt.isEmpty())
        item->addedAt = QDateTime::currentDateTime().toString(u"hh:mm:ss"_qs);
    item->errorMsg        = d[u"error_msg"_qs].toString().left(512);
    item->bytesDownloaded = d[u"bytes_downloaded"_qs].toInt(0);
    item->bytesTotal      = d[u"bytes_total"_qs].toInt(0);
    item->tempPath        = d[u"temp_path"_qs].toString();
    item->finalPath       = d[u"final_path"_qs].toString();
    item->isPlaylist      = d[u"is_playlist"_qs].toBool(false);
    return item;
}

// ─────────────────────────────────────────────
//  Queue persistence
// ─────────────────────────────────────────────
static const QSet<DownloadStatus> skipStatuses = {
    DownloadStatus::Completed, DownloadStatus::Cancelled
};

QList<DownloadItem*> loadQueue() {
    const QString& path = appPaths().queueDbFile;
    if (!QFileInfo::exists(path)) return {};
    if (QFileInfo(path).size() > QUEUE_DB_MAX_BYTES) return {};

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return {};

    QList<DownloadItem*> items;
    QSet<QString> seen;
    for (const auto& v : doc.array()) {
        if (!v.isObject()) continue;
        DownloadItem* item = itemFromJson(v.toObject());
        if (!item || item->id.isEmpty() || seen.contains(item->id)) {
            delete item;
            continue;
        }
        seen.insert(item->id);
        items.append(item);
        if (items.size() >= QUEUE_MAX_ENTRIES) break;
    }
    return items;
}

void saveQueue(const QList<DownloadItem*>& items) {
    QJsonArray arr;
    for (const auto* item : items) {
        if (skipStatuses.contains(item->status)) continue;
        arr.append(itemToJson(item));
    }
    atomicWriteText(appPaths().queueDbFile,
                    QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

// ─────────────────────────────────────────────
//  Schedule persistence
// ─────────────────────────────────────────────
QList<ScheduledItem> loadSchedule() {
    const QString& path = appPaths().scheduleFile;
    if (!QFileInfo::exists(path)) return {};
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray()) return {};
    QList<ScheduledItem> out;
    for (const auto& v : doc.array()) {
        if (v.isObject())
            out.append(ScheduledItem::fromJson(v.toObject()));
    }
    return out;
}

void saveSchedule(const QList<ScheduledItem>& items) {
    QJsonArray arr;
    for (const auto& it : items)
        arr.append(it.toJson());
    atomicWriteText(appPaths().scheduleFile,
                    QJsonDocument(arr).toJson(QJsonDocument::Indented));
}

} // namespace Persistence
