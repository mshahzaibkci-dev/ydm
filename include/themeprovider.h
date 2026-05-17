#pragma once
#include <QString>
#include <QMap>

// ─────────────────────────────────────────────
//  Palette key names (same as Python _DARK_PALETTE / _LIGHT_PALETTE)
// ─────────────────────────────────────────────
struct Palette {
    QString bg_primary, bg_secondary, card, hover;
    QString accent, accent_hover, accent2;
    QString success, warning, danger;
    QString text, text_sec, border, border2;
    QString input_bg, input_focus, glow;
    QString sidebar_top, sidebar_bot, dim;
};

class ThemeProvider {
public:
    static const Palette& dark();
    static const Palette& light();

    // Returns the active palette (starts as dark)
    static const Palette& active();

    // Switches between dark and light; rebuilds and returns new stylesheet
    static QString applyDark();
    static QString applyLight();

    static bool isDark();

    // Build a Qt Style Sheet from any palette
    static QString buildStyleSheet(const Palette& p);

private:
    static bool s_isDark;
};
