#include "widgets/scheduledialog.h"
#include "constants.h"
#include "themeprovider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDateTime>

ScheduleDialog::ScheduleDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(460);

    const Palette& c = ThemeProvider::active();

    auto* root = new QFrame(this);
    root->setObjectName(u"card"_qs);
    root->setStyleSheet(QStringLiteral(
        "QFrame#card { background: %1; border: 1px solid %2; border-radius: 16px; "
        "border-top: 3px solid %3; }"
    ).arg(c.card).arg(c.border2).arg(c.accent));

    auto* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->addWidget(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(16);

    // Title
    auto* titleLbl = new QLabel(u"⏰  Schedule Download"_qs);
    titleLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 16px; font-weight: 700; background: transparent; border: none;"
    ).arg(c.text));
    lay->addWidget(titleLbl);

    // Grid of fields
    auto* grid = new QGridLayout;
    grid->setSpacing(10);
    grid->setColumnStretch(1, 1);

    auto addField = [&](int row, const QString& label, QWidget* w) {
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 12px; font-weight: 600; background: transparent; border: none;"
        ).arg(c.text_sec));
        grid->addWidget(lbl, row, 0);
        grid->addWidget(w, row, 1);
    };

    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(u"https://…"_qs);
    addField(0, u"URL"_qs, m_urlEdit);

    m_fmtCombo = new QComboBox;
    for (const auto& opt : allFormatOptions())
        m_fmtCombo->addItem(opt.label);
    addField(1, u"Format"_qs, m_fmtCombo);

    m_calendar = new QCalendarWidget;
    m_calendar->setMinimumDate(QDate::currentDate());
    m_calendar->setMaximumWidth(400);
    grid->addWidget(new QLabel(u"Date"_qs), 2, 0, Qt::AlignTop);
    grid->addWidget(m_calendar, 2, 1);

    m_timeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600));
    m_timeEdit->setDisplayFormat(u"HH:mm"_qs);
    addField(3, u"Time"_qs, m_timeEdit);

    m_noteEdit = new QLineEdit;
    m_noteEdit->setPlaceholderText(u"Optional note…"_qs);
    m_noteEdit->setMaxLength(120);
    addField(4, u"Note"_qs, m_noteEdit);

    lay->addLayout(grid);

    // Status label
    m_statusLbl = new QLabel;
    m_statusLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; background: transparent; border: none;").arg(c.danger));
    m_statusLbl->hide();
    lay->addWidget(m_statusLbl);

    // Buttons
    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto* cancelBtn = new QPushButton(u"Cancel"_qs);
    cancelBtn->setObjectName(u"btnSecondary"_qs);
    cancelBtn->setMinimumHeight(36);
    cancelBtn->setMinimumWidth(90);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto* addBtn = new QPushButton(u"Schedule"_qs);
    addBtn->setObjectName(u"btnAdd"_qs);
    addBtn->setMinimumHeight(36);
    addBtn->setMinimumWidth(100);
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &ScheduleDialog::onAccept);
    btnRow->addWidget(addBtn);

    lay->addLayout(btnRow);
}

void ScheduleDialog::setUrl(const QString& url) {
    if (m_urlEdit) m_urlEdit->setText(url);
}

void ScheduleDialog::setFormatKeys(const QStringList& keys) {
    if (!m_fmtCombo) return;
    m_fmtCombo->clear();
    for (const auto& k : keys) m_fmtCombo->addItem(k);
}

void ScheduleDialog::onAccept() {
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) {
        m_statusLbl->setText(u"Please enter a URL."_qs);
        m_statusLbl->show();
        return;
    }

    QDate date = m_calendar->selectedDate();
    QTime time = m_timeEdit->time();
    QDateTime dt(date, time);

    if (dt <= QDateTime::currentDateTime()) {
        m_statusLbl->setText(u"Scheduled time must be in the future."_qs);
        m_statusLbl->show();
        return;
    }

    QString fmtKey = m_fmtCombo->currentText();
    m_result = ScheduledItem::create(url, fmtKey, dt, m_noteEdit->text().trimmed());
    m_statusLbl->hide();
    accept();
}
