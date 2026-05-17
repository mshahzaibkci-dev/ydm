#include "mainwindow.h"
#include "constants.h"
#include "utils.h"
#include "persistence.h"
#include "themeprovider.h"
#include "iconhelper.h"
#include "widgets/ydmdialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QStatusBar>
#include <QFont>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <winreg.h>
#endif

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings(new QSettings(u"RAINAX"_qs, u"DownloadManager"_qs, this))
    , m_manager(new DownloadManager(this))
{
    setWindowTitle(u"RAINAX – Download Manager"_qs);
    setMinimumSize(1080, 680);
    resize(1280, 780);
    setWindowIcon(QIcon(makeTrayIconPixmap()));

    // Wire manager signals
    connect(m_manager, &DownloadManager::itemAdded,
            this, &MainWindow::onItemAdded);
    connect(m_manager, &DownloadManager::itemUpdated,
            this, &MainWindow::onItemUpdated);
    connect(m_manager, &DownloadManager::queueChanged,
            this, &MainWindow::refreshTable);
    connect(m_manager, &DownloadManager::logReceived,
            this, &MainWindow::onLogLine);

    // ── Phase 1: tray only (instant) ──────────────────────────────────────
    setupTray();

    // ── Phase 2: deferred startup ─────────────────────────────────────────
    QTimer::singleShot(0,    this, &MainWindow::deferredBuildUI);
    QTimer::singleShot(150,  this, &MainWindow::deferredRestoreSettings);
    QTimer::singleShot(300,  this, &MainWindow::deferredLoadData);
    QTimer::singleShot(500,  this, &MainWindow::setupApiServer);
    QTimer::singleShot(800,  this, &MainWindow::deferredStartTicker);
    QTimer::singleShot(1000, this, &MainWindow::deferredLoadSchedule);
}

MainWindow::~MainWindow() {
    if (m_apiServer)  { m_apiServer->stop();  m_apiServer->deleteLater(); }
    if (m_instServer) { m_instServer->stop(); m_instServer->deleteLater(); }
    if (m_loader && m_loader->isRunning()) m_loader->wait(2000);
}

// ─────────────────────────────────────────────
//  Deferred startup sequence
// ─────────────────────────────────────────────
void MainWindow::deferredBuildUI() {
    buildUI();
    applyDarkTheme();
}

void MainWindow::deferredRestoreSettings() {
    if (!m_uiInitialized) return;
    restoreSettings();

    QByteArray geom = m_settings->value(u"window_geometry"_qs).toByteArray();
    if (!geom.isEmpty()) {
        try { restoreGeometry(geom); } catch (...) {}
    }

    bool expanded = m_settings->value(u"sidebar_expanded"_qs, true).toBool();
    if (!expanded) {
        m_sidebarExpanded = true;   // force animation to run
        animateSidebar(false);
    }
}

void MainWindow::deferredLoadData() {
    if (!m_uiInitialized) {
        QTimer::singleShot(150, this, &MainWindow::deferredLoadData);
        return;
    }
    m_loader = new BackgroundLoader(this);
    connect(m_loader, &BackgroundLoader::historyReady,
            this, &MainWindow::onHistoryLoaded);
    connect(m_loader, &BackgroundLoader::queueReady,
            this, &MainWindow::onQueueLoaded);
    connect(m_loader, &QThread::finished,
            m_loader, &QObject::deleteLater);
    m_loader->start();
}

void MainWindow::deferredStartTicker() {
    m_tick = new QTimer(this);
    m_tick->setInterval(1000);
    connect(m_tick, &QTimer::timeout, this, &MainWindow::tickIndicators);
    m_tick->start();
}

void MainWindow::deferredLoadSchedule() {
    if (!m_uiInitialized) {
        QTimer::singleShot(150, this, &MainWindow::deferredLoadSchedule);
        return;
    }
    m_scheduledItems = Persistence::loadSchedule();
    refreshScheduleTable();

    // Scheduler ticker (check every 30s)
    auto* schedTick = new QTimer(this);
    schedTick->setInterval(30000);
    connect(schedTick, &QTimer::timeout, [this]() {
        QDateTime now = QDateTime::currentDateTime();
        bool changed = false;
        for (auto& si : m_scheduledItems) {
            if (!si.fired && si.scheduledDt <= now) {
                si.fired = true;
                changed  = true;
                // Enqueue the scheduled download
                QString folder = m_folderLabel ? m_folderLabel->text()
                                               : QDir::homePath();
                QString catDir = makeCategorizedOutputDir(folder, {}, si.fmtKey);
                auto* item = new DownloadItem(DownloadItem::create(si.url, catDir, si.fmtKey));
                m_manager->add(item);
                if (m_tray)
                    m_tray->showMessage(u"RAINAX"_qs,
                        QStringLiteral("Scheduled download started: %1").arg(si.url.left(60)),
                        QSystemTrayIcon::Information, 4000);
            }
        }
        if (changed) {
            Persistence::saveSchedule(m_scheduledItems);
            refreshScheduleTable();
        }
    });
    schedTick->start();
}

// ─────────────────────────────────────────────
//  Tray setup
// ─────────────────────────────────────────────
void MainWindow::setupTray() {
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(makeTrayIconPixmap()));
    m_tray->setToolTip(u"RAINAX – Download Manager"_qs);

    auto* trayMenu = new QMenu;
    auto* aShow = new QAction(u"Show RAINAX"_qs, this);
    connect(aShow, &QAction::triggered, this, &MainWindow::_bringToFront);
    auto* aHide = new QAction(u"Hide RAINAX"_qs, this);
    connect(aHide, &QAction::triggered, this, &QWidget::hide);

    m_actionStartup = new QAction(u"Run at Windows Startup"_qs, this);
    m_actionStartup->setCheckable(true);
    m_actionStartup->setChecked(startupEnabled());
    connect(m_actionStartup, &QAction::toggled, this, &MainWindow::toggleStartup);

    auto* aQuit = new QAction(u"Quit"_qs, this);
    connect(aQuit, &QAction::triggered, this, &MainWindow::quitApp);

    trayMenu->addAction(aShow);
    trayMenu->addAction(aHide);
    trayMenu->addSeparator();
    trayMenu->addAction(m_actionStartup);
    trayMenu->addSeparator();
    trayMenu->addAction(aQuit);

    m_tray->setContextMenu(trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayActivated);
    m_tray->show();
}

// ─────────────────────────────────────────────
//  Tray / window management
// ─────────────────────────────────────────────
void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        _bringToFront();
}

void MainWindow::_bringToFront() {
    if (!m_uiInitialized) {
        QTimer::singleShot(100, this, &MainWindow::_bringToFront);
        return;
    }
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent* ev) {
    if (m_forceQuit) {
        ev->accept();
        return;
    }
    // IDM-style: hide to tray instead of closing
    hide();
    if (!m_trayNotified && m_tray) {
        m_tray->showMessage(u"RAINAX"_qs,
            u"RAINAX is still running in the system tray."_qs,
            QSystemTrayIcon::Information, 3000);
        m_trayNotified = true;
    }
    ev->ignore();
}

void MainWindow::quitApp() {
    m_forceQuit = true;
    m_manager->cancelAll();
    if (m_uiInitialized) saveSettings();
    Persistence::saveQueue(m_manager->queue);
    if (m_apiServer) m_apiServer->stop();
    QTimer::singleShot(400, qApp, &QApplication::quit);
}

// ─────────────────────────────────────────────
//  API server
// ─────────────────────────────────────────────
extern void setGlobalApiToken(const QString&);

void MainWindow::setupApiServer() {
    setGlobalApiToken(loadOrCreateApiToken());
    m_apiBridge = new APIBridge(this);
    connect(m_apiBridge, &APIBridge::downloadRequested,
            this, &MainWindow::onApiDownloadRequest);
    m_apiServer = new APIServerThread(m_apiBridge, this);
    m_apiServer->setManagerRef(m_manager);
    m_apiServer->start();
    QTimer::singleShot(1000, this, &MainWindow::reportApiStatus);
}

void MainWindow::reportApiStatus() {
    if (!m_uiInitialized) {
        QTimer::singleShot(200, this, &MainWindow::reportApiStatus);
        return;
    }
    const Palette& c = ThemeProvider::active();
    if (m_apiServer->errorString().isEmpty()) {
        statusBar()->showMessage(
            QStringLiteral("API ready  ·  http://%1:%2")
            .arg(QLatin1StringView(API_HOST)).arg(API_PORT));
        if (m_apiStatusLabel) {
            m_apiStatusLabel->setText(QStringLiteral("Online  ·  :%1").arg(API_PORT));
            m_apiStatusLabel->setStyleSheet(
                QStringLiteral("color: %1; font-size: 11px; font-weight: 600; background: transparent;")
                .arg(c.success));
        }
    } else {
        statusBar()->showMessage(
            QStringLiteral("API server failed: %1").arg(m_apiServer->errorString()));
        if (m_apiStatusLabel) {
            m_apiStatusLabel->setText(u"Offline"_qs);
            m_apiStatusLabel->setStyleSheet(
                QStringLiteral("color: %1; font-size: 11px; font-weight: 600; background: transparent;")
                .arg(c.danger));
        }
    }
}

// ─────────────────────────────────────────────
//  API download request (from bridge)
// ─────────────────────────────────────────────
void MainWindow::onApiDownloadRequest(const QString& url,
                                      const QString& title,
                                      const QString& quality)
{
    if (!m_uiInitialized) {
        QTimer::singleShot(200, [this, url, title, quality]() {
            onApiDownloadRequest(url, title, quality);
        });
        return;
    }
    if (!isSafeRemoteUrl(url)) {
        m_tray->showMessage(u"RAINAX"_qs,
            u"URL failed security validation."_qs,
            QSystemTrayIcon::Warning, 4000);
        return;
    }
    if (m_manager->urlInQueue(url)) {
        m_tray->showMessage(u"RAINAX"_qs,
            QStringLiteral("Already in queue:\n%1").arg(url.left(80)),
            QSystemTrayIcon::Information, 3000);
        return;
    }
    QString fmtKey = qualityMapLookup(quality);
    QString folder = m_folderLabel ? m_folderLabel->text() : QDir::homePath();
    if (!QFileInfo::exists(folder)) {
        QDir().mkpath(folder);
    }
    QString catDir = makeCategorizedOutputDir(folder, {}, fmtKey);
    auto* item = new DownloadItem(DownloadItem::create(url, catDir, fmtKey));
    m_manager->add(item);
    _bringToFront();
    switchPage(PAGE_QUEUE);
    QString disp = title.isEmpty() ? url.left(60) : title;
    m_tray->showMessage(u"RAINAX"_qs,
        QStringLiteral("Added: %1").arg(disp),
        QSystemTrayIcon::Information, 4000);
    updateStatusBar();
}

// ─────────────────────────────────────────────
//  Windows startup registry
// ─────────────────────────────────────────────
bool MainWindow::startupEnabled() const {
#ifdef Q_OS_WIN
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_READ, &key) != ERROR_SUCCESS)
        return false;
    DWORD type;
    DWORD size = 0;
    bool found = RegQueryValueExW(key, L"RAINAX", nullptr, &type, nullptr, &size)
                 == ERROR_SUCCESS;
    RegCloseKey(key);
    return found;
#else
    return false;
#endif
}

void MainWindow::toggleStartup(bool checked) {
    toggleStartupRegistry(checked);
}

void MainWindow::toggleStartupRegistry(bool enable) {
#ifdef Q_OS_WIN
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &key) != ERROR_SUCCESS)
        return;
    if (enable) {
        QString exePath = QApplication::applicationFilePath().replace('/', '\\');
        QString val = QStringLiteral("\"%1\" --tray").arg(exePath);
        std::wstring wval = val.toStdWString();
        RegSetValueExW(key, L"RAINAX", 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(wval.c_str()),
                       static_cast<DWORD>((wval.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, L"RAINAX");
    }
    RegCloseKey(key);
#endif
}

// ─────────────────────────────────────────────
//  Theme
// ─────────────────────────────────────────────
void MainWindow::applyDarkTheme() {
    QString ss = ThemeProvider::applyDark();
    qApp->setStyleSheet(ss);
    clearIconCache();
    if (m_settings) m_settings->setValue(u"theme"_qs, u"dark"_qs);
}

void MainWindow::applyLightTheme() {
    QString ss = ThemeProvider::applyLight();
    qApp->setStyleSheet(ss);
    clearIconCache();
    if (m_settings) m_settings->setValue(u"theme"_qs, u"light"_qs);
}

// ─────────────────────────────────────────────
//  Settings save/restore
// ─────────────────────────────────────────────
void MainWindow::saveSettings() {
    m_settings->setValue(u"window_geometry"_qs, saveGeometry());
    m_settings->setValue(u"sidebar_expanded"_qs, m_sidebarExpanded);
    if (m_folderLabel)
        m_settings->setValue(u"download_folder"_qs, m_folderLabel->text());
    if (m_fmtCombo)
        m_settings->setValue(u"format"_qs, m_fmtCombo->currentText());
}

void MainWindow::restoreSettings() {
    QString folder = m_settings->value(
        u"download_folder"_qs,
        QDir::homePath() + u"/Downloads"_qs).toString();
    if (m_folderLabel) m_folderLabel->setText(folder);

    QString fmt = m_settings->value(u"format"_qs, u"Best Quality (auto)"_qs).toString();
    if (m_fmtCombo) {
        int idx = m_fmtCombo->findText(fmt);
        if (idx >= 0) m_fmtCombo->setCurrentIndex(idx);
    }

    QString theme = m_settings->value(u"theme"_qs, u"dark"_qs).toString();
    if (theme == u"light"_qs) applyLightTheme();
    // dark is already applied by deferredBuildUI
}

// ─────────────────────────────────────────────
//  Page switching
// ─────────────────────────────────────────────
void MainWindow::switchPage(int page) {
    if (m_contentStack) m_contentStack->setCurrentIndex(page);
    for (int i = 0; i < m_sidebarButtons.size(); ++i)
        m_sidebarButtons[i]->setChecked(i == page);
}
