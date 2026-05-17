#include "widgets/ydmdialog.h"
#include "themeprovider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMouseEvent>

YDMDialog::YDMDialog(QWidget* parent, Kind kind,
                     const QString& title, const QString& message)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setMinimumWidth(380);
    setMaximumWidth(520);

    const Palette& c = ThemeProvider::active();

    // Accent + icon based on kind
    QString accentColor, iconChar;
    switch (kind) {
        case Kind::Error:    accentColor = c.danger;  iconChar = u"✕"_qs; break;
        case Kind::Warning:  accentColor = c.warning; iconChar = u"⚠"_qs; break;
        case Kind::Question: accentColor = c.accent;  iconChar = u"?"_qs; break;
        default:             accentColor = c.accent;  iconChar = u"ℹ"_qs; break;
    }

    // Root frame
    auto* root = new QFrame(this);
    root->setObjectName(u"card"_qs);
    root->setStyleSheet(QStringLiteral(
        "QFrame#card {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 16px;"
        "  border-top: 3px solid %3;"
        "}"
    ).arg(c.card).arg(c.border2).arg(accentColor));

    auto* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->addWidget(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(16);

    // Icon + title row
    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(14);

    auto* iconLbl = new QLabel(iconChar);
    iconLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 22px; font-weight: 700; "
        "background: transparent; border: none; }").arg(accentColor));
    topRow->addWidget(iconLbl);

    auto* titleLbl = new QLabel(title);
    titleLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 15px; font-weight: 700; "
        "background: transparent; border: none; }").arg(c.text));
    titleLbl->setWordWrap(true);
    topRow->addWidget(titleLbl, 1);
    lay->addLayout(topRow);

    // Message
    auto* msgLbl = new QLabel(message);
    msgLbl->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 13px; "
        "background: transparent; border: none; }").arg(c.text_sec));
    msgLbl->setWordWrap(true);
    lay->addWidget(msgLbl);

    // Buttons
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();

    if (kind == Kind::Question) {
        auto* cancelBtn = new QPushButton(u"Cancel"_qs);
        cancelBtn->setObjectName(u"btnSecondary"_qs);
        cancelBtn->setMinimumHeight(36);
        cancelBtn->setMinimumWidth(90);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        connect(cancelBtn, &QPushButton::clicked, this, &YDMDialog::onCancel);
        btnRow->addWidget(cancelBtn);

        auto* okBtn = new QPushButton(u"Confirm"_qs);
        okBtn->setObjectName(u"btnAdd"_qs);
        okBtn->setMinimumHeight(36);
        okBtn->setMinimumWidth(90);
        okBtn->setCursor(Qt::PointingHandCursor);
        connect(okBtn, &QPushButton::clicked, this, &YDMDialog::onOk);
        btnRow->addWidget(okBtn);
    } else {
        auto* okBtn = new QPushButton(u"OK"_qs);
        okBtn->setObjectName(u"btnAdd"_qs);
        okBtn->setMinimumHeight(36);
        okBtn->setMinimumWidth(90);
        okBtn->setCursor(Qt::PointingHandCursor);
        connect(okBtn, &QPushButton::clicked, this, &YDMDialog::onOk);
        btnRow->addWidget(okBtn);
    }
    lay->addLayout(btnRow);
}

void YDMDialog::onOk()     { m_accepted = true;  accept(); }
void YDMDialog::onCancel() { m_accepted = false; reject(); }

void YDMDialog::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::LeftButton)
        m_dragPos = ev->globalPosition().toPoint() - frameGeometry().topLeft();
}

void YDMDialog::mouseMoveEvent(QMouseEvent* ev) {
    if (ev->buttons() & Qt::LeftButton)
        move(ev->globalPosition().toPoint() - m_dragPos);
}

// Static convenience methods
void YDMDialog::showError(QWidget* parent, const QString& title, const QString& msg) {
    YDMDialog d(parent, Kind::Error, title, msg); d.exec();
}
void YDMDialog::showWarning(QWidget* parent, const QString& title, const QString& msg) {
    YDMDialog d(parent, Kind::Warning, title, msg); d.exec();
}
void YDMDialog::showInfo(QWidget* parent, const QString& title, const QString& msg) {
    YDMDialog d(parent, Kind::Info, title, msg); d.exec();
}
bool YDMDialog::showQuestion(QWidget* parent, const QString& title, const QString& msg) {
    YDMDialog d(parent, Kind::Question, title, msg);
    d.exec();
    return d.resultYes();
}
