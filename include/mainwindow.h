#pragma once
#include "downloadmanager.h"
#include "downloaditem.h"
#include "scheduleitem.h"
#include "apiserver.h"
#include "instanceserver.h"
#include "widgets/statusbadge.h"
#include "widgets/glossyprogressbar.h"
#include "widgets/statchip.h"
#include "widgets/statbox.h"
#include "widgets/sidebarbutton.h"
#include "widgets/backgroundloader.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QToolButton>
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QVariantMap>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Called by InstanceServer from any thread
    Q_INVOKABLE void _bringToFront();

protected:
    void closeEvent(QCloseEvent* ev) override;

private slots:
    // Deferred startup sequence
    void deferredBuildUI();
    void deferredRestoreSettings();
    void deferredLoadData();
    void deferredStartTicker();
    void deferredLoadSchedule();
    void setupApiServer();
    void reportApiStatus();

    // Tray
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void quitApp();

    // Download manager signals
    void onItemAdded(DownloadItem* item);
    void onItemUpdated(DownloadItem* item);
    void refreshTable();
    void onLogLine(const QString& id, const QString& line);

    // Background loader
    void onHistoryLoaded(const QList<QVariantMap>& entries);
    void onQueueLoaded(const QList<DownloadItem*>& items);

    // UI actions
    void addDownload();
    void onTableSelectionChanged();
    void onApiDownloadRequest(const QString& url,
                              const QString& title,
                              const QString& quality);

    // Toolbar actions
    void actionPause();
    void actionResume();
    void actionCancel();
    void actionClear();
    void actionClearHistory();

    // Theme
    void applyDarkTheme();
    void applyLightTheme();

    // Sidebar
    void toggleSidebar();
    void switchPage(int page);

    // Settings
    void saveSettings();
    void restoreSettings();

    // Status bar + ticker
    void tickIndicators();
    void updateStatusBar();

    // History
    void filterHistory(const QString& text);
    void populateHistoryTable();

    // Schedule
    void addScheduledDownload();
    void removeScheduledDownload();
    void refreshScheduleTable();

    // Startup registry (Windows)
    void toggleStartup(bool checked);
    void toggleStartupRegistry(bool enable);

    // yt-dlp / ffmpeg update
    void updateYtdlp();
    void updateFfmpeg();

    // Folder browse
    void browseFolder();

    // Log view
    void clearLogView();

private:
    // ── UI Build ──────────────────────────────────────────────────────────
    void buildUI();
    QWidget*  makeSidebar();
    QWidget*  makeHeader();
    QWidget*  makeQueuePage();
    QWidget*  makeCompletedPage();
    QWidget*  makeHistoryPage();
    QWidget*  makeLogsPage();
    QWidget*  makeSettingsPage();
    QWidget*  makeSchedulePage();
    QWidget*  makeInputPanel();
    void      animateSidebar(bool expand);

    // ── Table helpers ─────────────────────────────────────────────────────
    void insertTableRow(DownloadItem* item);
    void updateTableRow(DownloadItem* item);
    void appendToCompletedTable(DownloadItem* item);
    void appendToHistory(DownloadItem* item);
    QString selectedId() const;

    // ── Misc ──────────────────────────────────────────────────────────────
    QString aggregateSpeedStr() const;
    bool    startupEnabled() const;
    void    setupTray();

    // ── Page indices ──────────────────────────────────────────────────────
    enum Page {
        PAGE_QUEUE     = 0,
        PAGE_COMPLETED = 1,
        PAGE_HISTORY   = 2,
        PAGE_LOGS      = 3,
        PAGE_SETTINGS  = 4,
        PAGE_SCHEDULE  = 5,
    };

    // ── Column indices ────────────────────────────────────────────────────
    enum Col {
        COL_ID       = 0,
        COL_URL      = 1,
        COL_FORMAT   = 2,
        COL_FILENAME = 3,
        COL_SIZE     = 4,
        COL_SPEED    = 5,
        COL_ETA      = 6,
        COL_PROGRESS = 7,
        COL_STATUS   = 8,
    };

    static constexpr int SIDEBAR_EXPANDED  = 220;
    static constexpr int SIDEBAR_COLLAPSED = 52;

    // ── Owned objects ─────────────────────────────────────────────────────
    QSettings*       m_settings;
    DownloadManager* m_manager;
    QSystemTrayIcon* m_tray       = nullptr;
    APIServerThread* m_apiServer  = nullptr;
    APIBridge*       m_apiBridge  = nullptr;
    InstanceServer*  m_instServer = nullptr;
    BackgroundLoader* m_loader    = nullptr;
    QTimer*          m_tick       = nullptr;

    // ── UI widgets (persistent references) ───────────────────────────────
    QWidget*        m_sidebarWidget    = nullptr;
    bool            m_sidebarExpanded  = true;
    QPropertyAnimation* m_sidebarAnim = nullptr;

    QStackedWidget* m_contentStack = nullptr;
    QStackedWidget* m_queueStack   = nullptr;     // empty-state / table switch
    QStackedWidget* m_completedStack = nullptr;

    QTableWidget*   m_table          = nullptr;
    QTableWidget*   m_completedTable = nullptr;
    QTableWidget*   m_historyTable   = nullptr;
    QTableWidget*   m_scheduleTable  = nullptr;
    QTextEdit*      m_logView        = nullptr;

    QLineEdit*      m_urlEdit        = nullptr;
    QComboBox*      m_fmtCombo       = nullptr;
    QLabel*         m_folderLabel    = nullptr;
    QCheckBox*      m_playlistCheck  = nullptr;

    QLineEdit*      m_historySearch  = nullptr;

    // Header stat boxes
    StatBox*  m_statActive = nullptr;
    StatBox*  m_statQueue  = nullptr;
    StatBox*  m_statDone   = nullptr;
    StatBox*  m_statSpeed  = nullptr;

    // Live indicator
    QFrame*   m_livePill   = nullptr;
    QLabel*   m_liveDot    = nullptr;
    QLabel*   m_liveLabel  = nullptr;

    // API status label (settings page)
    QLabel*   m_apiStatusLabel = nullptr;

    // Sidebar buttons (for exclusive check)
    QList<SidebarButton*>  m_sidebarButtons;
    QToolButton*           m_sidebarToggleBtn = nullptr;
    QLabel*                m_brandLabel       = nullptr;
    QLabel*                m_brandLogo        = nullptr;

    // Settings page widgets
    QLabel*   m_ytdlpVerLabel  = nullptr;
    QLabel*   m_ffmpegVerLabel = nullptr;
    QCheckBox* m_startupCheck  = nullptr;
    QLabel*   m_ytdlpUpdateStatus = nullptr;
    QLabel*   m_ffmpegUpdateStatus = nullptr;

    // ── Per-row widget maps ───────────────────────────────────────────────
    QHash<QString, int>               m_rowMap;
    QHash<QString, GlossyProgressBar*> m_progressBars;
    QHash<QString, StatusBadge*>       m_statusBadges;
    QHash<QString, StatChip*>          m_speedChips;
    QHash<QString, StatChip*>          m_etaChips;
    QHash<QString, int>               m_completedRows;

    // ── State ─────────────────────────────────────────────────────────────
    QString              m_selectedItemId;
    QList<QVariantMap>   m_historyEntries;
    QList<ScheduledItem> m_scheduledItems;
    bool                 m_forceQuit      = false;
    bool                 m_trayNotified   = false;
    bool                 m_uiInitialized  = false;
    QAction*             m_actionStartup  = nullptr;
};
