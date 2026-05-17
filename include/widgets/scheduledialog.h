#pragma once
#include "scheduleitem.h"
#include <QDialog>
#include <QCalendarWidget>
#include <QTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

// Schedule-a-download dialog (Python: ScheduleDialog)
class ScheduleDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScheduleDialog(QWidget* parent = nullptr);

    // Returns the newly created ScheduledItem (only valid if exec() == Accepted)
    ScheduledItem result() const { return m_result; }

    void setUrl(const QString& url);
    void setFormatKeys(const QStringList& keys);

private slots:
    void onAccept();

private:
    ScheduledItem m_result;
    QLineEdit*       m_urlEdit    = nullptr;
    QComboBox*       m_fmtCombo   = nullptr;
    QCalendarWidget* m_calendar   = nullptr;
    QTimeEdit*       m_timeEdit   = nullptr;
    QLineEdit*       m_noteEdit   = nullptr;
    QLabel*          m_statusLbl  = nullptr;
};
