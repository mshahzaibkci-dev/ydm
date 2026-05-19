#include "mainwindow.h"
#include "constants.h"
#include "themeprovider.h"
#include "iconhelper.h"
#include "widgets/sidebarbutton.h"
#include "widgets/statbox.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QSplitter>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>

// ─────────────────────────────────────────────
//  IDM-style toolbar button helper
// ─────────────────────────────────────────────
static QToolButton* makeIdmBtn(const QString& label,
                                const QString& iconFile,
                                const QString& tip,
                                bool danger = false)
{
    auto* btn = new QToolButton;
    btn->setObjectName(danger ? u"idmToolBtnDanger"_qs : u"idmToolBtn"_qs);
    btn->setIcon(loadIcon(iconFile, 28));
    btn->setIconSize(QSize(28, 28));
    btn->setText(label);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setMinimumWidth(70);
    btn->setMinimumHeight(58);
    return btn;
}

// ─────────────────────────────────────────────
//  buildUI() – assembles the complete IDM-style shell
// ─────────────────────────────────────────────
void MainWindow::buildUI() {

    // ── Menu bar ─────────────────────────────────────────────────────────
    {
        QMenuBar* mb = menuBar();
        mb->setNativeMenuBar(false);  // force Qt menu bar on all platforms

        // Downloads menu
        QMenu* mDownloads = mb->addMenu(u"&Downloads"_qs);

        auto* actAdd = mDownloads->addAction(u"&Add URL…"_qs);
        actAdd->setShortcut(QKeySequence(u"Ctrl+N"_qs));
        connect(actAdd, &QAction::triggered, this, &MainWindow::addDownload);

        mDownloads->addSeparator();

        auto* actResume = mDownloads->addAction(u"&Resume"_qs);
        actResume->setShortcut(QKeySequence(u"Ctrl+R"_qs));
        connect(actResume, &QAction::triggered, this, &MainWindow::actionResume);

        auto* actPause = mDownloads->addAction(u"&Pause"_qs);
        actPause->setShortcut(QKeySequence(u"Ctrl+P"_qs));
        connect(actPause, &QAction::triggered, this, &MainWindow::actionPause);

        auto* actStop = mDownloads->addAction(u"S&top"_qs);
        connect(actStop, &QAction::triggered, this, &MainWindow::actionCancel);

        mDownloads->addSeparator();

        auto* actClear = mDownloads->addAction(u"Clear &Finished"_qs);
        connect(actClear, &QAction::triggered, this, &MainWindow::actionClear);

        mDownloads->addSeparator();

        auto* actExit = mDownloads->addAction(u"E&xit"_qs);
        actExit->setShortcut(QKeySequence(u"Ctrl+Q"_qs));
        connect(actExit, &QAction::triggered, this, &MainWindow::quitApp);

        // View menu
        QMenu* mView = mb->addMenu(u"&View"_qs);

        auto* actQueue = mView->addAction(u"&Downloads Queue"_qs);
        connect(actQueue, &QAction::triggered, [this]() { switchPage(PAGE_QUEUE); });

        auto* actCompleted = mView->addAction(u"&Completed"_qs);
        connect(actCompleted, &QAction::triggered, [this]() { switchPage(PAGE_COMPLETED); });

        auto* actHistory = mView->addAction(u"D&ownload History"_qs);
        connect(actHistory, &QAction::triggered, [this]() { switchPage(PAGE_HISTORY); });

        auto* actLogs = mView->addAction(u"&Logs"_qs);
        connect(actLogs, &QAction::triggered, [this]() { switchPage(PAGE_LOGS); });

        mView->addSeparator();

        auto* actDark = mView->addAction(u"&Dark Theme"_qs);
        connect(actDark, &QAction::triggered, this, &MainWindow::applyDarkTheme);

        auto* actLight = mView->addAction(u"&Light Theme"_qs);
        connect(actLight, &QAction::triggered, this, &MainWindow::applyLightTheme);

        mView->addSeparator();

        auto* actToggle = mView->addAction(u"&Toggle Sidebar"_qs);
        actToggle->setShortcut(QKeySequence(u"Ctrl+B"_qs));
        connect(actToggle, &QAction::triggered, this, &MainWindow::toggleSidebar);

        // Tools menu
        QMenu* mTools = mb->addMenu(u"&Tools"_qs);

        auto* actSchedule = mTools->addAction(u"Task &Scheduler"_qs);
        connect(actSchedule, &QAction::triggered, [this]() { switchPage(PAGE_SCHEDULE); });

        mTools->addSeparator();

        auto* actUpdYtdlp = mTools->addAction(u"Update &yt-dlp"_qs);
        connect(actUpdYtdlp, &QAction::triggered, this, &MainWindow::updateYtdlp);

        auto* actUpdFfmpeg = mTools->addAction(u"Update &ffmpeg"_qs);
        connect(actUpdFfmpeg, &QAction::triggered, this, &MainWindow::updateFfmpeg);

        mTools->addSeparator();

        auto* actSettings = mTools->addAction(u"&Options / Settings"_qs);
        actSettings->setShortcut(QKeySequence(u"Ctrl+,"_qs));
        connect(actSettings, &QAction::triggered, [this]() { switchPage(PAGE_SETTINGS); });
    }

    // ── IDM-Style Main Toolbar ────────────────────────────────────────────
    {
        auto* tb = new QToolBar(u"Main Toolbar"_qs, this);
        tb->setObjectName(u"idmToolBar"_qs);
        tb->setMovable(false);
        tb->setIconSize(QSize(28, 28));
        tb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        addToolBar(Qt::TopToolBarArea, tb);

        // Add URL
        auto* btnAddUrl = makeIdmBtn(u"Add URL"_qs, u"download.png"_qs, u"Add a new download URL (Ctrl+N)"_qs);
        connect(btnAddUrl, &QToolButton::clicked, this, &MainWindow::addDownload);
        tb->addWidget(btnAddUrl);

        tb->addSeparator();

        // Resume
        auto* btnResume = makeIdmBtn(u"Resume"_qs, u"resume.png"_qs, u"Resume selected download (Ctrl+R)"_qs);
        connect(btnResume, &QToolButton::clicked, this, &MainWindow::actionResume);
        tb->addWidget(btnResume);

        // Pause
        auto* btnPause = makeIdmBtn(u"Pause"_qs, u"pause.png"_qs, u"Pause selected download (Ctrl+P)"_qs);
        connect(btnPause, &QToolButton::clicked, this, &MainWindow::actionPause);
        tb->addWidget(btnPause);

        // Stop
        auto* btnStop = makeIdmBtn(u"Stop"_qs, u"cancel.png"_qs, u"Cancel selected download"_qs, true);
        connect(btnStop, &QToolButton::clicked, this, &MainWindow::actionCancel);
        tb->addWidget(btnStop);

        tb->addSeparator();

        // Delete/Clear
        auto* btnDelete = makeIdmBtn(u"Clear Done"_qs, u"cancel.png"_qs, u"Clear finished downloads"_qs, true);
        connect(btnDelete, &QToolButton::clicked, this, &MainWindow::actionClear);
        tb->addWidget(btnDelete);

        tb->addSeparator();

        // Scheduler
        auto* btnScheduler = makeIdmBtn(u"Scheduler"_qs, u"schedule.png"_qs, u"Task Scheduler"_qs);
        connect(btnScheduler, &QToolButton::clicked, [this]() { switchPage(PAGE_SCHEDULE); });
        tb->addWidget(btnScheduler);

        // Settings
        auto* btnSettings = makeIdmBtn(u"Options"_qs, u"settings.png"_qs, u"Options / Settings (Ctrl+,)"_qs);
        connect(btnSettings, &QToolButton::clicked, [this]() { switchPage(PAGE_SETTINGS); });
        tb->addWidget(btnSettings);

        tb->addSeparator();

        // Theme toggle buttons (compact, no label)
        auto* btnDark = new QToolButton;
        btnDark->setObjectName(u"idmToolBtn"_qs);
        btnDark->setText(u"Dark"_qs);
        btnDark->setIcon(loadIcon(u"settings.png"_qs, 18));
        btnDark->setIconSize(QSize(18, 18));
        btnDark->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btnDark->setToolTip(u"Switch to dark theme"_qs);
        btnDark->setMinimumWidth(54);
        btnDark->setMinimumHeight(58);
        connect(btnDark, &QToolButton::clicked, this, &MainWindow::applyDarkTheme);
        tb->addWidget(btnDark);

        auto* btnLight = new QToolButton;
        btnLight->setObjectName(u"idmToolBtn"_qs);
        btnLight->setText(u"Light"_qs);
        btnLight->setIcon(loadIcon(u"settings.png"_qs, 18));
        btnLight->setIconSize(QSize(18, 18));
        btnLight->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btnLight->setToolTip(u"Switch to light theme"_qs);
        btnLight->setMinimumWidth(54);
        btnLight->setMinimumHeight(58);
        connect(btnLight, &QToolButton::clicked, this, &MainWindow::applyLightTheme);
        tb->addWidget(btnLight);
    }

    // ── Root widget ───────────────────────────────────────────────────────
    auto* central = new QWidget(this);
    central->setObjectName(u"rootFrame"_qs);
    setCentralWidget(central);

    auto* rootH = new QHBoxLayout(central);
    rootH->setContentsMargins(0, 0, 0, 0);
    rootH->setSpacing(0);

    // Sidebar
    m_sidebarWidget = makeSidebar();
    m_sidebarWidget->setFixedWidth(SIDEBAR_EXPANDED);
    rootH->addWidget(m_sidebarWidget);

    // Right side: header + content stacked vertically
    auto* rightCol = new QWidget;
    auto* rightV   = new QVBoxLayout(rightCol);
    rightV->setContentsMargins(0, 0, 0, 0);
    rightV->setSpacing(0);

    // Header (stat boxes strip)
    auto* header = makeHeader();
    rightV->addWidget(header);

    // Content stack
    m_contentStack = new QStackedWidget;
    m_contentStack->setObjectName(u"contentArea"_qs);

    // Add pages in PAGE_* order
    m_contentStack->addWidget(makeQueuePage());       // 0
    m_contentStack->addWidget(makeCompletedPage());   // 1
    m_contentStack->addWidget(makeHistoryPage());     // 2
    m_contentStack->addWidget(makeLogsPage());        // 3
    m_contentStack->addWidget(makeSettingsPage());    // 4
    m_contentStack->addWidget(makeSchedulePage());    // 5

    rightV->addWidget(m_contentStack, 1);
    rootH->addWidget(rightCol, 1);

    // ── Status bar ────────────────────────────────────────────────────────
    statusBar()->setFixedHeight(24);

    // ── Sidebar animation ─────────────────────────────────────────────────
    m_sidebarAnim = new QPropertyAnimation(m_sidebarWidget, "maximumWidth", this);
    m_sidebarAnim->setDuration(240);
    m_sidebarAnim->setEasingCurve(QEasingCurve::OutCubic);
    auto* minAnim = new QPropertyAnimation(m_sidebarWidget, "minimumWidth", this);
    minAnim->setDuration(240);
    minAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_uiInitialized = true;

    // Default page
    switchPage(PAGE_QUEUE);
}

// ─────────────────────────────────────────────
//  Sidebar
// ─────────────────────────────────────────────
QWidget* MainWindow::makeSidebar() {
    auto* sidebar = new QFrame;
    sidebar->setObjectName(u"sidebar"_qs);

    auto* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // ── Brand row ─────────────────────────────────────────────────────────
    auto* brand = new QFrame;
    brand->setFixedHeight(56);
    brand->setStyleSheet(u"border: none; background: transparent;"_qs);
    auto* brandH = new QHBoxLayout(brand);
    brandH->setContentsMargins(14, 0, 8, 0);
    brandH->setSpacing(10);

    m_brandLogo = new QLabel;
    m_brandLogo->setPixmap(makeLogoPixmap(28));
    m_brandLogo->setFixedSize(28, 28);
    brandH->addWidget(m_brandLogo);

    m_brandLabel = new QLabel(u"<b>RAIN<span style='color:#1a6fc4'>AX</span></b>"_qs);
    m_brandLabel->setStyleSheet(
        u"font-size: 15px; font-weight: 700; letter-spacing: 0.5px; "
        u"background: transparent; border: none;"_qs);
    m_brandLabel->setTextFormat(Qt::RichText);
    brandH->addWidget(m_brandLabel, 1);

    m_sidebarToggleBtn = new QToolButton;
    m_sidebarToggleBtn->setObjectName(u"sideToggleBtn"_qs);
    m_sidebarToggleBtn->setText(u"◀"_qs);
    m_sidebarToggleBtn->setFixedSize(26, 26);
    m_sidebarToggleBtn->setToolTip(u"Collapse sidebar"_qs);
    connect(m_sidebarToggleBtn, &QToolButton::clicked,
            this, &MainWindow::toggleSidebar);
    brandH->addWidget(m_sidebarToggleBtn);

    lay->addWidget(brand);

    // Separator
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(u"background: rgba(128,128,128,0.20); border: none; max-height: 1px;"_qs);
    lay->addWidget(sep);
    lay->addSpacing(4);

    // ── Category section label ────────────────────────────────────────────
    auto* catLabel = new QLabel(u"  CATEGORIES"_qs);
    catLabel->setStyleSheet(
        u"font-size: 9px; font-weight: 700; letter-spacing: 1.2px; "
        u"color: rgba(136,146,168,0.70); background: transparent; border: none; "
        u"padding: 6px 0 3px 16px;"_qs);
    catLabel->setObjectName(u"catLabel"_qs);
    lay->addWidget(catLabel);

    // ── Nav buttons ───────────────────────────────────────────────────────
    struct NavEntry { QString label; QString iconFile; int page; };
    const QList<NavEntry> entries = {
        { u"Downloads"_qs,  u"download.png"_qs,   PAGE_QUEUE     },
        { u"Completed"_qs,  u"completed.png"_qs,  PAGE_COMPLETED },
        { u"History"_qs,    u"history.png"_qs,    PAGE_HISTORY   },
        { u"Logs"_qs,       u"logs.png"_qs,       PAGE_LOGS      },
        { u"Scheduler"_qs,  u"schedule.png"_qs,   PAGE_SCHEDULE  },
        { u"Settings"_qs,   u"settings.png"_qs,   PAGE_SETTINGS  },
    };

    m_sidebarButtons.clear();
    for (const auto& e : entries) {
        auto* btn = new SidebarButton(e.label,
                                      loadIcon(e.iconFile, 18),
                                      sidebar);
        const int page = e.page;
        connect(btn, &QPushButton::clicked, [this, page]() {
            switchPage(page);
        });
        lay->addWidget(btn);
        m_sidebarButtons.append(btn);
    }

    lay->addStretch(1);

    // ── Live indicator ────────────────────────────────────────────────────
    m_livePill = new QFrame;
    m_livePill->setObjectName(u"livePill"_qs);
    m_livePill->setFixedHeight(32);
    m_livePill->setStyleSheet(
        u"QFrame#livePill { background: rgba(62,165,114,0.10); "
        u"border: 1px solid rgba(62,165,114,0.25); border-radius: 4px; "
        u"margin: 0px 10px 10px 10px; }"_qs);
    auto* pillH = new QHBoxLayout(m_livePill);
    pillH->setContentsMargins(10, 0, 10, 0);
    pillH->setSpacing(6);

    m_liveDot = new QLabel(u"●"_qs);
    m_liveDot->setStyleSheet(u"color: #3ea572; font-size: 9px; background: transparent; border: none;"_qs);
    pillH->addWidget(m_liveDot);

    m_liveLabel = new QLabel(u"Idle"_qs);
    m_liveLabel->setStyleSheet(u"color: #3ea572; font-size: 11px; font-weight: 600; background: transparent; border: none;"_qs);
    pillH->addWidget(m_liveLabel, 1);

    lay->addWidget(m_livePill);

    return sidebar;
}

// ─────────────────────────────────────────────
//  Header  –  stat boxes strip only (toolbar is now QToolBar above)
// ─────────────────────────────────────────────
QWidget* MainWindow::makeHeader() {
    auto* header = new QFrame;
    header->setObjectName(u"header"_qs);
    header->setFixedHeight(62);

    auto* hlay = new QHBoxLayout(header);
    hlay->setContentsMargins(20, 0, 20, 0);
    hlay->setSpacing(14);

    // Title column
    auto* titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto* title = new QLabel(u"RAINAX Download Manager"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    title->setStyleSheet(u"font-size: 14px; font-weight: 700; letter-spacing: 0.2px;"_qs);
    auto* sub = new QLabel(u"Fast  ·  Reliable  ·  yt-dlp powered"_qs);
    sub->setObjectName(u"sectionSub"_qs);
    titleCol->addWidget(title);
    titleCol->addWidget(sub);
    hlay->addLayout(titleCol);

    hlay->addStretch(1);

    // Stat boxes
    m_statActive = new StatBox(u"ACTIVE"_qs, u"0"_qs, u"#1a6fc4"_qs);
    m_statQueue  = new StatBox(u"QUEUED"_qs, u"0"_qs, u"#d4921a"_qs);
    m_statDone   = new StatBox(u"DONE"_qs,   u"0"_qs, u"#3ea572"_qs);
    m_statSpeed  = new StatBox(u"SPEED"_qs,  u"—"_qs, u"#4aa3f0"_qs);
    hlay->addWidget(m_statActive);
    hlay->addWidget(m_statQueue);
    hlay->addWidget(m_statDone);
    hlay->addWidget(m_statSpeed);

    return header;
}

// ─────────────────────────────────────────────
//  Input panel (URL entry)
// ─────────────────────────────────────────────
QWidget* MainWindow::makeInputPanel() {
    auto* panel = new QFrame;
    panel->setObjectName(u"inputPanel"_qs);
    panel->setMaximumHeight(100);

    auto* lay = new QHBoxLayout(panel);
    lay->setContentsMargins(20, 12, 20, 12);
    lay->setSpacing(10);

    // URL input
    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(
        u"Paste URL here  (YouTube, Vimeo, TikTok, direct files…)"_qs);
    m_urlEdit->setMinimumHeight(36);
    m_urlEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_urlEdit, &QLineEdit::returnPressed,
            this, &MainWindow::addDownload);
    lay->addWidget(m_urlEdit, 3);

    // Format combo
    m_fmtCombo = new QComboBox;
    m_fmtCombo->setMinimumHeight(36);
    m_fmtCombo->setMinimumWidth(180);
    for (const auto& opt : allFormatOptions())
        m_fmtCombo->addItem(opt.label);
    lay->addWidget(m_fmtCombo, 1);

    // Folder
    auto* folderBtn = new QToolButton;
    folderBtn->setObjectName(u"toolBtn"_qs);
    folderBtn->setIcon(loadIcon(u"folder.png"_qs, 16));
    folderBtn->setIconSize(QSize(16, 16));
    folderBtn->setFixedHeight(36);
    folderBtn->setMinimumWidth(36);
    folderBtn->setToolTip(u"Browse download folder"_qs);
    connect(folderBtn, &QToolButton::clicked,
            this, &MainWindow::browseFolder);
    lay->addWidget(folderBtn);

    m_folderLabel = new QLabel(QDir::homePath() + u"/Downloads"_qs);
    m_folderLabel->setObjectName(u"sectionSub"_qs);
    m_folderLabel->setToolTip(u"Download folder"_qs);
    m_folderLabel->setMaximumWidth(200);
    m_folderLabel->setWordWrap(false);
    m_folderLabel->setStyleSheet(
        u"font-size: 11px; background: transparent; border: none;"_qs);
    lay->addWidget(m_folderLabel);

    // Playlist checkbox
    m_playlistCheck = new QCheckBox(u"Playlist"_qs);
    lay->addWidget(m_playlistCheck);

    // Add button
    auto* addBtn = new QPushButton(u"+ Download"_qs);
    addBtn->setObjectName(u"btnAdd"_qs);
    addBtn->setMinimumHeight(36);
    addBtn->setMinimumWidth(120);
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::addDownload);
    lay->addWidget(addBtn);

    return panel;
}

// ─────────────────────────────────────────────
//  Queue page
// ─────────────────────────────────────────────
QWidget* MainWindow::makeQueuePage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(20, 14, 20, 14);
    lay->setSpacing(10);

    // Input panel
    lay->addWidget(makeInputPanel());

    // In-page secondary toolbar
    auto* toolbar = new QFrame;
    toolbar->setObjectName(u"toolbar"_qs);
    toolbar->setFixedHeight(44);
    auto* tbH = new QHBoxLayout(toolbar);
    tbH->setContentsMargins(0, 0, 0, 0);
    tbH->setSpacing(6);

    auto makeToolBtn = [&](const QString& text, const QString& icon,
                           const QString& tip, auto slot) {
        auto* btn = new QToolButton;
        btn->setObjectName(u"toolBtn"_qs);
        btn->setIcon(loadIcon(icon, 15));
        btn->setIconSize(QSize(15, 15));
        btn->setText(text);
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setFixedHeight(32);
        btn->setToolTip(tip);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QToolButton::clicked, this, slot);
        return btn;
    };

    tbH->addWidget(makeToolBtn(u"▶ Resume"_qs, u"resume.png"_qs,
                               u"Resume selected"_qs, &MainWindow::actionResume));
    tbH->addWidget(makeToolBtn(u"⏸ Pause"_qs,  u"pause.png"_qs,
                               u"Pause selected"_qs,  &MainWindow::actionPause));

    auto* cancelBtn = new QToolButton;
    cancelBtn->setObjectName(u"toolBtnDanger"_qs);
    cancelBtn->setIcon(loadIcon(u"cancel.png"_qs, 15));
    cancelBtn->setIconSize(QSize(15, 15));
    cancelBtn->setText(u"✕ Cancel"_qs);
    cancelBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    cancelBtn->setFixedHeight(32);
    cancelBtn->setToolTip(u"Cancel selected"_qs);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QToolButton::clicked, this, &MainWindow::actionCancel);
    tbH->addWidget(cancelBtn);

    tbH->addStretch(1);

    auto* clearBtn = new QToolButton;
    clearBtn->setObjectName(u"toolBtnDanger"_qs);
    clearBtn->setText(u"Clear Finished"_qs);
    clearBtn->setFixedHeight(32);
    clearBtn->setToolTip(u"Remove completed/cancelled entries"_qs);
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QToolButton::clicked, this, &MainWindow::actionClear);
    tbH->addWidget(clearBtn);

    lay->addWidget(toolbar);

    // Downloads table (inside a stacked widget for empty state)
    m_queueStack = new QStackedWidget;

    // Empty state page (index 0)
    auto* emptyPage = new QWidget;
    auto* emptyV = new QVBoxLayout(emptyPage);
    emptyV->addStretch();
    auto* emptyLbl = new QLabel(u"📥  No downloads yet\nPaste a URL above and press Download"_qs);
    emptyLbl->setAlignment(Qt::AlignCenter);
    emptyLbl->setStyleSheet(u"color: #888888; font-size: 14px; background: transparent; border: none;"_qs);
    emptyLbl->setWordWrap(true);
    emptyV->addWidget(emptyLbl);
    emptyV->addStretch();
    m_queueStack->addWidget(emptyPage);   // index 0

    // Table page (index 1)
    m_table = new QTableWidget(0, 9);
    m_table->setObjectName(u"downloadTable"_qs);
    m_table->setHorizontalHeaderLabels({
        u"ID"_qs, u"URL"_qs, u"Format"_qs, u"Filename"_qs,
        u"Size"_qs, u"Speed"_qs, u"ETA"_qs, u"Progress"_qs, u"Status"_qs
    });
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(COL_URL, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_FILENAME, QHeaderView::Stretch);
    m_table->setColumnWidth(COL_ID,       60);
    m_table->setColumnWidth(COL_FORMAT,  150);
    m_table->setColumnWidth(COL_SIZE,     80);
    m_table->setColumnWidth(COL_SPEED,    90);
    m_table->setColumnWidth(COL_ETA,      70);
    m_table->setColumnWidth(COL_PROGRESS,130);
    m_table->setColumnWidth(COL_STATUS,  130);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setRowHeight(0, 50);
    m_table->setShowGrid(false);
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onTableSelectionChanged);
    m_queueStack->addWidget(m_table);    // index 1

    lay->addWidget(m_queueStack, 1);
    return page;
}

// ─────────────────────────────────────────────
//  Completed page
// ─────────────────────────────────────────────
QWidget* MainWindow::makeCompletedPage() {
    auto* page = new QWidget;
    auto* lay  = new QVBoxLayout(page);
    lay->setContentsMargins(20, 14, 20, 14);
    lay->setSpacing(12);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(u"✅  Completed Downloads"_qs);
    title->setObjectName(u"sectionTitle"_qs);
    titleRow->addWidget(title);
    titleRow->addStretch();
    lay->addLayout(titleRow);

    m_completedStack = new QStackedWidget;

    auto* emptyPage = new QWidget;
    auto* emptyV = new QVBoxLayout(emptyPage);
    emptyV->addStretch();
    auto* emptyLbl = new QLabel(u"✅  No completed downloads yet"_qs);
    emptyLbl->setAlignment(Qt::AlignCenter);
    emptyLbl->setStyleSheet(u"color: #888888; font-size: 14px; background: transparent; border: none;"_qs);
    emptyV->addWidget(emptyLbl);
    emptyV->addStretch();
    m_completedStack->addWidget(emptyPage);  // 0

    m_completedTable = new QTableWidget(0, 4);
    m_completedTable->setHorizontalHeaderLabels({
        u"Filename"_qs, u"Format"_qs, u"Size"_qs, u"Folder"_qs
    });
    m_completedTable->horizontalHeader()->setStretchLastSection(true);
    m_completedTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_completedTable->setColumnWidth(1, 160);
    m_completedTable->setColumnWidth(2, 90);
    m_completedTable->verticalHeader()->setVisible(false);
    m_completedTable->setAlternatingRowColors(true);
    m_completedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_completedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_completedTable->setShowGrid(false);
    m_completedStack->addWidget(m_completedTable);  // 1

    lay->addWidget(m_completedStack, 1);
    return page;
}

// ─────────────────────────────────────────────
//  Sidebar animation
// ─────────────────────────────────────────────
void MainWindow::toggleSidebar() {
    animateSidebar(!m_sidebarExpanded);
}

void MainWindow::animateSidebar(bool expand) {
    if (expand == m_sidebarExpanded) return;
    m_sidebarExpanded = expand;

    int fromW = expand ? SIDEBAR_COLLAPSED : SIDEBAR_EXPANDED;
    int toW   = expand ? SIDEBAR_EXPANDED  : SIDEBAR_COLLAPSED;

    if (m_sidebarAnim) {
        m_sidebarAnim->stop();
        m_sidebarAnim->setStartValue(fromW);
        m_sidebarAnim->setEndValue(toW);
        m_sidebarAnim->start();
    } else {
        m_sidebarWidget->setFixedWidth(toW);
    }

    bool textVisible = expand;
    if (m_brandLabel)  m_brandLabel->setVisible(textVisible);
    if (m_liveLabel)   m_liveLabel->setVisible(textVisible);
    if (m_sidebarToggleBtn) {
        m_sidebarToggleBtn->setText(expand ? u"◀"_qs : u"▶"_qs);
        m_sidebarToggleBtn->setToolTip(expand ? u"Collapse sidebar"_qs : u"Expand sidebar"_qs);
    }
    for (auto* btn : std::as_const(m_sidebarButtons))
        btn->setExpanded(expand);
}

// ─────────────────────────────────────────────
//  Folder browse
// ─────────────────────────────────────────────
void MainWindow::browseFolder() {
    QString start = m_folderLabel ? m_folderLabel->text() : QDir::homePath();
    QString dir = QFileDialog::getExistingDirectory(
        this, u"Select Download Folder"_qs, start);
    if (!dir.isEmpty() && m_folderLabel) {
        m_folderLabel->setText(dir);
        if (m_settings) m_settings->setValue(u"download_folder"_qs, dir);
    }
}
