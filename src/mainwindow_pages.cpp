#include "mainwindow.h"
#include "constants.h"
#include "utils.h"
#include "themeprovider.h"
#include "iconhelper.h"
#include "widgets/ydmdialog.h"
#include "widgets/scheduledialog.h"
#include "ytdlpupdateworker.h"
#include "ffmpegupdateworker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QScrollArea>
#include <QSpacerItem>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>

// ─────────────────────────────────────────────
//  History page
// ─────────────────────────────────────────────
QWidget* MainWindow::makeHistoryPage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(24, 18, 24, 18);
    lay->setSpacing(14);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(u"🕓  Download History"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    titleRow->addWidget(title);
    titleRow->addStretch();

    auto* clearHistBtn = new QPushButton(u"Clear History"_qs);
    clearHistBtn->setObjectName(u"btnDanger"_qs);
    clearHistBtn->setMinimumHeight(32);
    clearHistBtn->setCursor(Qt::PointingHandCursor);
    connect(clearHistBtn, &QPushButton::clicked,
            this, &MainWindow::actionClearHistory);
    titleRow->addWidget(clearHistBtn);
    lay->addLayout(titleRow);

    // Search bar
    m_historySearch = new QLineEdit;
    m_historySearch->setObjectName(u"searchField"_qs);
    m_historySearch->setPlaceholderText(u"🔍  Search history…"_qs);
    m_historySearch->setFixedHeight(36);
    connect(m_historySearch, &QLineEdit::textChanged,
            this, &MainWindow::filterHistory);
    lay->addWidget(m_historySearch);

    // Table
    m_historyTable = new QTableWidget(0, 4);
    m_historyTable->setHorizontalHeaderLabels({
        u"Time"_qs, u"Filename"_qs, u"URL"_qs, u"Status"_qs
    });
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_historyTable->setColumnWidth(0, 110);
    m_historyTable->setColumnWidth(3, 110);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setShowGrid(false);
    lay->addWidget(m_historyTable, 1);

    return page;
}

// ─────────────────────────────────────────────
//  Logs page
// ─────────────────────────────────────────────
QWidget* MainWindow::makeLogsPage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(24, 18, 24, 18);
    lay->setSpacing(14);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(u"📋  Download Logs"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    titleRow->addWidget(title);
    titleRow->addStretch();

    auto* clearBtn = new QPushButton(u"Clear"_qs);
    clearBtn->setObjectName(u"btnSecondary"_qs);
    clearBtn->setMinimumHeight(32);
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearLogView);
    titleRow->addWidget(clearBtn);
    lay->addLayout(titleRow);

    m_logView = new QTextEdit;
    m_logView->setReadOnly(true);
    m_logView->setLineWrapMode(QTextEdit::WidgetWidth);
    m_logView->setMinimumHeight(400);
    lay->addWidget(m_logView, 1);

    return page;
}

// ─────────────────────────────────────────────
//  Settings page (full 1:1 with Python)
// ─────────────────────────────────────────────
QWidget* MainWindow::makeSettingsPage() {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(24, 18, 24, 32);
    lay->setSpacing(24);

    auto* title = new QLabel(u"⚙️  Settings"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    lay->addWidget(title);

    // ── Helper lambdas ────────────────────────────────────────────────────
    auto makeCard = [&]() -> QFrame* {
        auto* card = new QFrame;
        card->setObjectName(u"card"_qs);
        return card;
    };

    auto sectionTitle = [](const QString& t) {
        auto* lbl = new QLabel(t);
        lbl->setStyleSheet(
            u"font-size: 13px; font-weight: 700; color: #a89df8; "
            u"background: transparent; border: none; letter-spacing: 0.2px;"_qs);
        return lbl;
    };

    auto fieldLabel = [](const QString& t) {
        auto* lbl = new QLabel(t);
        lbl->setStyleSheet(
            u"font-size: 12px; font-weight: 600; color: #7a8fba; "
            u"background: transparent; border: none;"_qs);
        return lbl;
    };

    auto valueLbl = [](const QString& t) {
        auto* lbl = new QLabel(t);
        lbl->setStyleSheet(
            u"font-size: 12px; color: #eef2ff; background: transparent; border: none;"_qs);
        return lbl;
    };

    // ── 1. Dependencies ───────────────────────────────────────────────────
    {
        auto* card = makeCard();
        auto* vlay = new QVBoxLayout(card);
        vlay->setContentsMargins(20, 16, 20, 16);
        vlay->setSpacing(14);
        vlay->addWidget(sectionTitle(u"🔧  Dependencies"_qs));

        auto* grid = new QGridLayout;
        grid->setSpacing(10);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 0);
        grid->setColumnStretch(3, 0);

        // yt-dlp row
        grid->addWidget(fieldLabel(u"yt-dlp"_qs), 0, 0);
        m_ytdlpVerLabel = valueLbl(u"Checking…"_qs);
        grid->addWidget(m_ytdlpVerLabel, 0, 1);
        m_ytdlpUpdateStatus = new QLabel;
        m_ytdlpUpdateStatus->setStyleSheet(
            u"font-size: 11px; background: transparent; border: none;"_qs);
        grid->addWidget(m_ytdlpUpdateStatus, 0, 2);

        auto* ytdlpUpdateBtn = new QPushButton(u"Update yt-dlp"_qs);
        ytdlpUpdateBtn->setObjectName(u"btnSecondary"_qs);
        ytdlpUpdateBtn->setMinimumHeight(30);
        ytdlpUpdateBtn->setCursor(Qt::PointingHandCursor);
        connect(ytdlpUpdateBtn, &QPushButton::clicked,
                this, &MainWindow::updateYtdlp);
        grid->addWidget(ytdlpUpdateBtn, 0, 3);

        // ffmpeg row
        grid->addWidget(fieldLabel(u"ffmpeg"_qs), 1, 0);
        m_ffmpegVerLabel = valueLbl(u"Checking…"_qs);
        grid->addWidget(m_ffmpegVerLabel, 1, 1);
        m_ffmpegUpdateStatus = new QLabel;
        m_ffmpegUpdateStatus->setStyleSheet(
            u"font-size: 11px; background: transparent; border: none;"_qs);
        grid->addWidget(m_ffmpegUpdateStatus, 1, 2);

        auto* ffmpegUpdateBtn = new QPushButton(u"Update ffmpeg"_qs);
        ffmpegUpdateBtn->setObjectName(u"btnSecondary"_qs);
        ffmpegUpdateBtn->setMinimumHeight(30);
        ffmpegUpdateBtn->setCursor(Qt::PointingHandCursor);
        connect(ffmpegUpdateBtn, &QPushButton::clicked,
                this, &MainWindow::updateFfmpeg);
        grid->addWidget(ffmpegUpdateBtn, 1, 3);

        vlay->addLayout(grid);
        lay->addWidget(card);

        // Populate versions asynchronously
        QTimer::singleShot(600, [this]() {
            if (m_ytdlpVerLabel)  m_ytdlpVerLabel->setText(getYtdlpVersion());
            if (m_ffmpegVerLabel) m_ffmpegVerLabel->setText(getFfmpegVersion());
        });
    }

    // ── 2. API Server ─────────────────────────────────────────────────────
    {
        auto* card = makeCard();
        auto* vlay = new QVBoxLayout(card);
        vlay->setContentsMargins(20, 16, 20, 16);
        vlay->setSpacing(12);
        vlay->addWidget(sectionTitle(u"🔌  Browser Extension API"_qs));

        auto* rowH = new QHBoxLayout;
        rowH->addWidget(fieldLabel(u"Status"_qs));
        m_apiStatusLabel = new QLabel(u"Starting…"_qs);
        m_apiStatusLabel->setStyleSheet(
            u"font-size: 11px; font-weight: 600; background: transparent; border: none;"_qs);
        rowH->addWidget(m_apiStatusLabel, 1);
        vlay->addLayout(rowH);

        auto* tokenRow = new QHBoxLayout;
        tokenRow->addWidget(fieldLabel(u"Port"_qs));
        tokenRow->addWidget(valueLbl(QStringLiteral("%1").arg(API_PORT)), 1);
        vlay->addLayout(tokenRow);

        auto* helpLbl = new QLabel(
            u"The browser extension communicates with RAINAX via this local API.\n"
            u"Token is stored in  .ydm_api_token  next to the executable."_qs);
        helpLbl->setStyleSheet(
            u"font-size: 11px; color: #7a8fba; background: transparent; border: none;"_qs);
        helpLbl->setWordWrap(true);
        vlay->addWidget(helpLbl);

        lay->addWidget(card);
    }

    // ── 3. System ─────────────────────────────────────────────────────────
    {
        auto* card = makeCard();
        auto* vlay = new QVBoxLayout(card);
        vlay->setContentsMargins(20, 16, 20, 16);
        vlay->setSpacing(12);
        vlay->addWidget(sectionTitle(u"🖥  System"_qs));

        m_startupCheck = new QCheckBox(u"Run RAINAX at Windows startup"_qs);
        m_startupCheck->setChecked(startupEnabled());
        connect(m_startupCheck, &QCheckBox::toggled,
                this, &MainWindow::toggleStartup);
        vlay->addWidget(m_startupCheck);

        lay->addWidget(card);
    }

    // ── 4. About ──────────────────────────────────────────────────────────
    {
        auto* card = makeCard();
        auto* vlay = new QVBoxLayout(card);
        vlay->setContentsMargins(20, 16, 20, 16);
        vlay->setSpacing(8);
        vlay->addWidget(sectionTitle(u"ℹ  About RAINAX"_qs));

        auto* aboutLbl = new QLabel(
            u"<b>RAINAX Download Manager</b> v2.0<br>"
            u"Qt6 / C++20 edition<br>"
            u"<span style='color:#7a8fba;'>Powered by yt-dlp and ffmpeg.</span>"_qs);
        aboutLbl->setStyleSheet(
            u"font-size: 12px; color: #eef2ff; background: transparent; border: none;"_qs);
        aboutLbl->setTextFormat(Qt::RichText);
        aboutLbl->setWordWrap(true);
        vlay->addWidget(aboutLbl);

        lay->addWidget(card);
    }

    lay->addStretch();
    scroll->setWidget(page);
    return scroll;
}

// ─────────────────────────────────────────────
//  Schedule page
// ─────────────────────────────────────────────
QWidget* MainWindow::makeSchedulePage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(24, 18, 24, 18);
    lay->setSpacing(14);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(u"⏰  Scheduler"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    titleRow->addWidget(title);
    titleRow->addStretch();

    auto* addSchedBtn = new QPushButton(u"+ Schedule Download"_qs);
    addSchedBtn->setObjectName(u"btnAdd"_qs);
    addSchedBtn->setMinimumHeight(36);
    addSchedBtn->setMinimumWidth(160);
    addSchedBtn->setCursor(Qt::PointingHandCursor);
    connect(addSchedBtn, &QPushButton::clicked,
            this, &MainWindow::addScheduledDownload);
    titleRow->addWidget(addSchedBtn);

    auto* removeSchedBtn = new QPushButton(u"Remove Selected"_qs);
    removeSchedBtn->setObjectName(u"btnDanger"_qs);
    removeSchedBtn->setMinimumHeight(36);
    removeSchedBtn->setMinimumWidth(140);
    removeSchedBtn->setCursor(Qt::PointingHandCursor);
    connect(removeSchedBtn, &QPushButton::clicked,
            this, &MainWindow::removeScheduledDownload);
    titleRow->addWidget(removeSchedBtn);

    lay->addLayout(titleRow);

    // Info label
    auto* info = new QLabel(
        u"Downloads will be automatically queued at their scheduled time, "
        u"even if the window is hidden to the tray."_qs);
    info->setStyleSheet(
        u"color: #7a8fba; font-size: 12px; background: transparent; border: none;"_qs);
    info->setWordWrap(true);
    lay->addWidget(info);

    // Schedule table
    m_scheduleTable = new QTableWidget(0, 5);
    m_scheduleTable->setHorizontalHeaderLabels({
        u"Scheduled Time"_qs, u"URL"_qs, u"Format"_qs, u"Note"_qs, u"Status"_qs
    });
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_scheduleTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_scheduleTable->setColumnWidth(0, 140);
    m_scheduleTable->setColumnWidth(2, 160);
    m_scheduleTable->setColumnWidth(4, 90);
    m_scheduleTable->verticalHeader()->setVisible(false);
    m_scheduleTable->setAlternatingRowColors(true);
    m_scheduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_scheduleTable->setShowGrid(false);
    lay->addWidget(m_scheduleTable, 1);

    return page;
}

// ─────────────────────────────────────────────
//  Schedule table refresh
// ─────────────────────────────────────────────
void MainWindow::refreshScheduleTable() {
    if (!m_scheduleTable) return;
    m_scheduleTable->setRowCount(0);
    const Palette& c = ThemeProvider::active();
    for (const auto& si : std::as_const(m_scheduledItems)) {
        int row = m_scheduleTable->rowCount();
        m_scheduleTable->insertRow(row);

        auto cell = [&](int col, const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_scheduleTable->setItem(row, col, item);
        };

        cell(0, si.scheduledDt.toString(u"yyyy-MM-dd HH:mm"_qs));
        cell(1, si.url);
        cell(2, si.fmtKey);
        cell(3, si.note);
        cell(4, si.fired ? u"Fired"_qs : u"Pending"_qs);

        // Colour status cell
        if (auto* statusItem = m_scheduleTable->item(row, 4)) {
            statusItem->setForeground(
                si.fired ? QColor(c.success) : QColor(c.accent2));
        }
        m_scheduleTable->setRowHeight(row, 40);
    }
}

// ─────────────────────────────────────────────
//  Add scheduled download
// ─────────────────────────────────────────────
void MainWindow::addScheduledDownload() {
    ScheduleDialog dlg(this);
    if (m_urlEdit && !m_urlEdit->text().trimmed().isEmpty())
        dlg.setUrl(m_urlEdit->text().trimmed());

    QStringList keys;
    for (const auto& opt : allFormatOptions()) keys << opt.label;
    dlg.setFormatKeys(keys);

    if (dlg.exec() != QDialog::Accepted) return;

    ScheduledItem si = dlg.result();
    m_scheduledItems.append(si);
    Persistence::saveSchedule(m_scheduledItems);
    refreshScheduleTable();
}

// ─────────────────────────────────────────────
//  Remove scheduled download
// ─────────────────────────────────────────────
void MainWindow::removeScheduledDownload() {
    if (!m_scheduleTable) return;
    int row = m_scheduleTable->currentRow();
    if (row < 0 || row >= m_scheduledItems.size()) return;
    m_scheduledItems.removeAt(row);
    Persistence::saveSchedule(m_scheduledItems);
    refreshScheduleTable();
}

// ─────────────────────────────────────────────
//  Update yt-dlp
// ─────────────────────────────────────────────
void MainWindow::updateYtdlp() {
    if (!m_ytdlpUpdateStatus) return;
    const Palette& c = ThemeProvider::active();
    m_ytdlpUpdateStatus->setText(u"Updating…"_qs);
    m_ytdlpUpdateStatus->setStyleSheet(
        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
        .arg(c.warning));

    auto* worker = new YtdlpUpdateWorker(this);
    connect(worker, &YtdlpUpdateWorker::progressMsg,
            [this](const QString& msg) {
                if (m_ytdlpUpdateStatus) m_ytdlpUpdateStatus->setText(msg.left(80));
            });
    connect(worker, &YtdlpUpdateWorker::finishedOk,
            [this, &c](const QString& ver) {
                if (m_ytdlpVerLabel) m_ytdlpVerLabel->setText(ver);
                if (m_ytdlpUpdateStatus) {
                    m_ytdlpUpdateStatus->setText(u"Updated ✓"_qs);
                    m_ytdlpUpdateStatus->setStyleSheet(
                        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
                        .arg(c.success));
                }
            });
    connect(worker, &YtdlpUpdateWorker::finishedErr,
            [this, &c](const QString& err) {
                if (m_ytdlpUpdateStatus) {
                    m_ytdlpUpdateStatus->setText(u"Error ✕"_qs);
                    m_ytdlpUpdateStatus->setStyleSheet(
                        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
                        .arg(c.danger));
                }
                YDMDialog::showError(this, u"yt-dlp Update Error"_qs, err);
            });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

// ─────────────────────────────────────────────
//  Update ffmpeg
// ─────────────────────────────────────────────
void MainWindow::updateFfmpeg() {
    if (!m_ffmpegUpdateStatus) return;
    const Palette& c = ThemeProvider::active();
    m_ffmpegUpdateStatus->setText(u"Downloading…"_qs);
    m_ffmpegUpdateStatus->setStyleSheet(
        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
        .arg(c.warning));

    auto* worker = new FfmpegUpdateWorker(this);
    connect(worker, &FfmpegUpdateWorker::progressMsg,
            [this](const QString& msg) {
                if (m_ffmpegUpdateStatus) m_ffmpegUpdateStatus->setText(msg.left(80));
            });
    connect(worker, &FfmpegUpdateWorker::finishedOk,
            [this, &c](const QString& ver) {
                if (m_ffmpegVerLabel) m_ffmpegVerLabel->setText(ver);
                if (m_ffmpegUpdateStatus) {
                    m_ffmpegUpdateStatus->setText(u"Updated ✓"_qs);
                    m_ffmpegUpdateStatus->setStyleSheet(
                        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
                        .arg(c.success));
                }
            });
    connect(worker, &FfmpegUpdateWorker::finishedErr,
            [this, &c](const QString& err) {
                if (m_ffmpegUpdateStatus) {
                    m_ffmpegUpdateStatus->setText(u"Error ✕"_qs);
                    m_ffmpegUpdateStatus->setStyleSheet(
                        QStringLiteral("color: %1; font-size: 11px; background: transparent; border: none;")
                        .arg(c.danger));
                }
                YDMDialog::showError(this, u"ffmpeg Update Error"_qs, err);
            });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}
