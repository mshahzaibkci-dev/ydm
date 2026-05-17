#include "scheduleitem.h"
#include "constants.h"
#include <QUuid>
#include <QJsonObject>

ScheduledItem ScheduledItem::create(const QString& url, const QString& fmtKey,
                                    const QDateTime& dt, const QString& note)
{
    ScheduledItem it;
    it.id          = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    it.url         = url;
    it.fmtKey      = isValidFormatKey(fmtKey) ? fmtKey : u"Best Quality (auto)"_qs;
    it.scheduledDt = dt;
    it.note        = note.left(120);
    it.fired       = false;
    it.addedAt     = QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm"_qs);
    return it;
}

QJsonObject ScheduledItem::toJson() const {
    QJsonObject o;
    o[u"id"_qs]           = id;
    o[u"url"_qs]          = url;
    o[u"fmt_key"_qs]      = fmtKey;
    o[u"scheduled_dt"_qs] = scheduledDt.toString(u"yyyy-MM-dd HH:mm"_qs);
    o[u"note"_qs]         = note;
    o[u"fired"_qs]        = fired;
    o[u"added_at"_qs]     = addedAt;
    return o;
}

ScheduledItem ScheduledItem::fromJson(const QJsonObject& obj) {
    ScheduledItem it;
    it.id     = obj[u"id"_qs].toString();
    it.url    = obj[u"url"_qs].toString();
    it.fmtKey = isValidFormatKey(obj[u"fmt_key"_qs].toString())
                    ? obj[u"fmt_key"_qs].toString()
                    : u"Best Quality (auto)"_qs;
    it.scheduledDt = QDateTime::fromString(
        obj[u"scheduled_dt"_qs].toString(), u"yyyy-MM-dd HH:mm"_qs);
    if (!it.scheduledDt.isValid())
        it.scheduledDt = QDateTime::currentDateTime();
    it.note     = obj[u"note"_qs].toString().left(120);
    it.fired    = obj[u"fired"_qs].toBool(false);
    it.addedAt  = obj[u"added_at"_qs].toString();
    if (it.id.isEmpty())
        it.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    return it;
}
