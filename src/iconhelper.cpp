#include "iconhelper.h"
#include "constants.h"
#include "themeprovider.h"
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFontMetrics>

// ─────────────────────────────────────────────
//  Caches
// ─────────────────────────────────────────────
static QHash<QString, QIcon>   s_iconCache;
static QHash<QString, QPixmap> s_pixmapCache;

static QString iconPath(const QString& name) {
    return QDir(appPaths().iconsDir).filePath(name);
}

static QPixmap blankPixmap(int size) {
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    return px;
}

QIcon loadIcon(const QString& name, int size) {
    QString key = name + '@' + QString::number(size);
    auto it = s_iconCache.find(key);
    if (it != s_iconCache.end()) return *it;

    QString path = iconPath(name);
    if (QFileInfo::exists(path)) {
        QPixmap px(path);
        if (!px.isNull()) {
            QPixmap scaled = px.scaled(size, size,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QIcon icon(scaled);
            s_iconCache.insert(key, icon);
            return icon;
        }
    }
    // Return transparent blank – never fall back to system icons
    QIcon blank(blankPixmap(size));
    s_iconCache.insert(key, blank);
    return blank;
}

QPixmap loadPixmap(const QString& name, int size) {
    QString key = name + '@' + QString::number(size);
    auto it = s_pixmapCache.find(key);
    if (it != s_pixmapCache.end()) return *it;

    QString path = iconPath(name);
    if (QFileInfo::exists(path)) {
        QPixmap px(path);
        if (!px.isNull()) {
            QPixmap scaled = px.scaled(size, size,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            s_pixmapCache.insert(key, scaled);
            return scaled;
        }
    }
    QPixmap blank = blankPixmap(size);
    s_pixmapCache.insert(key, blank);
    return blank;
}

// ─────────────────────────────────────────────
//  Tray icon: painted programmatically
// ─────────────────────────────────────────────
QPixmap makeTrayIconPixmap() {
    // Try to load from file first
    QString path = iconPath(u"tray.png"_qs);
    if (QFileInfo::exists(path)) {
        QPixmap px(path);
        if (!px.isNull())
            return px.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // Fallback: draw a purple circle with "R"
    QPixmap px(32, 32);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);

    // Background circle
    const Palette& pal = ThemeProvider::active();
    p.setBrush(QColor(pal.accent));
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 1, 30, 30);

    // Letter "R"
    p.setPen(QColor(u"#ffffff"_qs));
    QFont f(u"Segoe UI"_qs, 14, QFont::Bold);
    p.setFont(f);
    p.drawText(QRect(0, 0, 32, 32), Qt::AlignCenter, u"R"_qs);
    p.end();
    return px;
}

// ─────────────────────────────────────────────
//  Logo pixmap: gradient circle with "R"
// ─────────────────────────────────────────────
QPixmap makeLogoPixmap(int size) {
    QString path = iconPath(u"logo.png"_qs);
    if (QFileInfo::exists(path)) {
        QPixmap px(path);
        if (!px.isNull())
            return px.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);

    const Palette& pal = ThemeProvider::active();
    QLinearGradient grad(0, 0, size, size);
    grad.setColorAt(0.0, QColor(pal.accent));
    grad.setColorAt(1.0, QColor(pal.accent2));
    p.setBrush(QBrush(grad));
    p.setPen(Qt::NoPen);

    QPainterPath circle;
    circle.addEllipse(1, 1, size - 2, size - 2);
    p.drawPath(circle);

    p.setPen(QColor(u"#ffffff"_qs));
    QFont f(u"Segoe UI"_qs, size / 2, QFont::Black);
    p.setFont(f);
    p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, u"R"_qs);
    p.end();
    return px;
}

void clearIconCache() {
    s_iconCache.clear();
    s_pixmapCache.clear();
}
