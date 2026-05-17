#include "constants.h"
#include "instanceserver.h"
#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QSurfaceFormat>
#include <cstdlib>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

int main(int argc, char* argv[]) {
    // ── High-DPI support ─────────────────────────────────────────────────
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName(u"RAINAX"_qs);
    app.setApplicationVersion(u"2.0.0"_qs);
    app.setOrganizationName(u"RAINAX"_qs);
    app.setQuitOnLastWindowClosed(false);   // tray keeps app alive

    // ── Paths ────────────────────────────────────────────────────────────
    initAppPaths();

    // ── Single-instance check ────────────────────────────────────────────
    // Parse --tray flag (auto-minimise to tray at startup)
    bool startHidden = false;
    for (int i = 1; i < argc; ++i) {
        if (QLatin1StringView(argv[i]) == "--tray")
            startHidden = true;
    }

    // If another instance is already running, signal it and exit
    if (signalExistingInstance()) {
        return 0;
    }

    // ── Global font ──────────────────────────────────────────────────────
    QFont appFont(u"Segoe UI Variable"_qs, 10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);

    // ── Main window ──────────────────────────────────────────────────────
    MainWindow w;

    // Start the single-instance server (must pass window ref after creation)
    auto* instServer = new InstanceServer(&w, &w);
    instServer->start();

    if (!startHidden) {
        w.show();
    }

    return app.exec();
}
