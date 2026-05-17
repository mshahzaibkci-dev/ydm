#pragma once
#include <QIcon>
#include <QPixmap>
#include <QString>

// ─────────────────────────────────────────────
//  Icon / pixmap helpers  (cached, safe-load)
// ─────────────────────────────────────────────
QIcon   loadIcon(const QString& name, int size = 20);
QPixmap loadPixmap(const QString& name, int size = 20);
QPixmap makeTrayIconPixmap();
QPixmap makeLogoPixmap(int size = 28);

void clearIconCache();
