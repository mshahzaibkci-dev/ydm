#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct ScheduledItem {
    QString   id;
    QString   url;
    QString   fmtKey;
    QDateTime scheduledDt;
    QString   note;
    bool      fired    = false;
    QString   addedAt;

    static ScheduledItem create(const QString& url, const QString& fmtKey,
                                const QDateTime& dt, const QString& note = {});

    QJsonObject toJson() const;
    static ScheduledItem fromJson(const QJsonObject& obj);
};
