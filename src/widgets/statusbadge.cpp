#include "widgets/statusbadge.h"
#include "themeprovider.h"

// ── Status colour pairs (fg, bg) ── dark / light ──────────────────────────
static QPair<QString,QString> statusColors(DownloadStatus s) {
    bool dark = ThemeProvider::isDark();
    if (dark) {
        switch (s) {
            case DownloadStatus::Queued:     return { u"#4a5d7a"_qs, u"#0c1828"_qs };
            case DownloadStatus::Starting:   return { u"#818cf8"_qs, u"#0d1535"_qs };
            case DownloadStatus::Running:    return { u"#6366f1"_qs, u"#0d1535"_qs };
            case DownloadStatus::Paused:     return { u"#f59e0b"_qs, u"#1c1505"_qs };
            case DownloadStatus::Cancelling: return { u"#f43f5e"_qs, u"#1c0510"_qs };
            case DownloadStatus::Completed:  return { u"#10b981"_qs, u"#051c12"_qs };
            case DownloadStatus::Failed:     return { u"#f43f5e"_qs, u"#1c0510"_qs };
            case DownloadStatus::Cancelled:  return { u"#4a5d7a"_qs, u"#0c1828"_qs };
        }
    } else {
        switch (s) {
            case DownloadStatus::Queued:     return { u"#4a5680"_qs, u"#e8eaf2"_qs };
            case DownloadStatus::Starting:   return { u"#5b53d4"_qs, u"#ece9ff"_qs };
            case DownloadStatus::Running:    return { u"#4f42c8"_qs, u"#ece9ff"_qs };
            case DownloadStatus::Paused:     return { u"#b45309"_qs, u"#fef3c7"_qs };
            case DownloadStatus::Cancelling: return { u"#be123c"_qs, u"#ffe4e6"_qs };
            case DownloadStatus::Completed:  return { u"#065f46"_qs, u"#d1fae5"_qs };
            case DownloadStatus::Failed:     return { u"#be123c"_qs, u"#ffe4e6"_qs };
            case DownloadStatus::Cancelled:  return { u"#4a5680"_qs, u"#e8eaf2"_qs };
        }
    }
    return { u"#4a5d7a"_qs, u"#0c1828"_qs };
}

StatusBadge::StatusBadge(DownloadStatus status, QWidget* parent)
    : QLabel(parent), m_status(status)
{
    setAlignment(Qt::AlignCenter);
    setFixedHeight(22);
    setStatus(status);
}

void StatusBadge::setStatus(DownloadStatus status) {
    m_status = status;
    auto [fg, bg] = statusColors(status);
    setText(u"\u25cf "_qs + statusToString(status));
    setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background: %1;"
        "  color: %2;"
        "  border-radius: 11px;"
        "  padding: 2px 12px 2px 9px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  letter-spacing: 0.3px;"
        "}").arg(bg).arg(fg));
}
