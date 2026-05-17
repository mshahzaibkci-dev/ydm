#include "themeprovider.h"

bool ThemeProvider::s_isDark = true;

// ── Windows 11 Dark  ───────────────────────────────────────────────────────
//  Colours sourced from WinUI 3 / Fluent dark-mode design tokens.
//  Backgrounds follow the Mica layering model (Base -> Layer -> CardBackground).
const Palette& ThemeProvider::dark() {
    static const Palette p = {
        .bg_primary   = u"#202020"_qs,                // ApplicationPageBackgroundThemeBrush dark
        .bg_secondary = u"#1c1c1c"_qs,                // slightly deeper layer
        .card         = u"#2d2d2d"_qs,                // CardBackgroundFillColorDefaultBrush dark
        .hover        = u"#3a3a3a"_qs,                // SubtleFillColorSecondaryBrush dark
        .accent       = u"#0078d4"_qs,                // AccentFillColorDefaultBrush (Win11 blue)
        .accent_hover = u"#1687d9"_qs,                // AccentFillColorSecondaryBrush dark
        .accent2      = u"#60c1f8"_qs,                // lighter accent for text/icons on dark bg
        .success      = u"#6ccb5f"_qs,                // SystemFillColorSuccess dark
        .warning      = u"#fce100"_qs,                // SystemFillColorCaution dark
        .danger       = u"#ff99a4"_qs,                // SystemFillColorCritical dark
        .text         = u"#ffffff"_qs,                // TextFillColorPrimaryBrush dark
        .text_sec     = u"rgba(255,255,255,0.54)"_qs, // TextFillColorSecondaryBrush dark
        .border       = u"rgba(255,255,255,0.08)"_qs, // ControlStrokeColorDefaultBrush dark
        .border2      = u"rgba(255,255,255,0.18)"_qs, // stronger stroke for inputs/focus
        .input_bg     = u"#3b3b3b"_qs,                // ControlFillColorDefaultBrush dark
        .input_focus  = u"#454545"_qs,                // ControlFillColorSecondaryBrush dark
        .glow         = u"rgba(0,120,212,0.22)"_qs,
        .sidebar_top  = u"#272727"_qs,
        .sidebar_bot  = u"#1e1e1e"_qs,
        .dim          = u"rgba(255,255,255,0.28)"_qs, // TextFillColorDisabledBrush dark
    };
    return p;
}

// ── Windows 11 Light  ──────────────────────────────────────────────────────
//  Colours sourced from WinUI 3 / Fluent light-mode design tokens.
const Palette& ThemeProvider::light() {
    static const Palette p = {
        .bg_primary   = u"#f3f3f3"_qs,          // ApplicationPageBackgroundThemeBrush light
        .bg_secondary = u"#ebebeb"_qs,
        .card         = u"#ffffff"_qs,           // CardBackgroundFillColorDefaultBrush light
        .hover        = u"#f0f0f0"_qs,           // SubtleFillColorSecondaryBrush light
        .accent       = u"#0078d4"_qs,           // AccentFillColorDefaultBrush
        .accent_hover = u"#006cbe"_qs,           // AccentFillColorSecondaryBrush light
        .accent2      = u"#005a9e"_qs,           // darker accent for text on light bg
        .success      = u"#107c10"_qs,           // SystemFillColorSuccess light
        .warning      = u"#9d5d00"_qs,           // SystemFillColorCaution light
        .danger       = u"#c42b1c"_qs,           // SystemFillColorCritical light
        .text         = u"#1a1a1a"_qs,           // TextFillColorPrimaryBrush light
        .text_sec     = u"rgba(0,0,0,0.55)"_qs,  // TextFillColorSecondaryBrush light
        .border       = u"rgba(0,0,0,0.08)"_qs,  // ControlStrokeColorDefaultBrush light
        .border2      = u"rgba(0,0,0,0.20)"_qs,  // stronger stroke for inputs/focus
        .input_bg     = u"#ffffff"_qs,            // ControlFillColorDefaultBrush light
        .input_focus  = u"#f5f5f5"_qs,           // ControlFillColorSecondaryBrush light
        .glow         = u"rgba(0,120,212,0.14)"_qs,
        .sidebar_top  = u"#f0f0f0"_qs,
        .sidebar_bot  = u"#e5e5e5"_qs,
        .dim          = u"rgba(0,0,0,0.36)"_qs,  // TextFillColorDisabledBrush light
    };
    return p;
}

const Palette& ThemeProvider::active() {
    return s_isDark ? dark() : light();
}

bool ThemeProvider::isDark() { return s_isDark; }

QString ThemeProvider::applyDark()  { s_isDark = true;  return buildStyleSheet(dark()); }
QString ThemeProvider::applyLight() { s_isDark = false; return buildStyleSheet(light()); }

// ─────────────────────────────────────────────────────────────────────────────
//  Qt Style Sheet — Windows 11 Fluent Design
//
//  Arg map (same positional slots as the original — all callers unaffected):
//  %1  bg_primary    %2  bg_secondary  %3  card          %4  hover
//  %5  accent        %6  accent_hover  %7  accent2       %8  success
//  %9  warning       %10 danger        %11 text(dup)     %12 text
//  %13 text_sec      %14 border2       %15 input_bg      %16 input_focus
//  %17 glow          %18 border        %19 sidebar_top   %20 sidebar_bot
//  %21 dim
// ─────────────────────────────────────────────────────────────────────────────
QString ThemeProvider::buildStyleSheet(const Palette& c) {
    return QStringLiteral(
R"(
/* ── Global ──────────────────────────────────────────────────────────────── */
* {
    font-family: 'Segoe UI Variable', 'Segoe UI', Arial, sans-serif;
    font-size: 13px;
    outline: none;
}
QMainWindow, QWidget {
    background-color: %1;
    color: %12;
}
QFrame#rootFrame { background-color: %1; }

/* ══ SIDEBAR ════════════════════════════════════════════════════════════════ */
QFrame#sidebar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 %19, stop:1 %20);
    border: none;
    border-right: 1px solid %18;
}

/* ══ HEADER ═════════════════════════════════════════════════════════════════ */
QFrame#header {
    background-color: %3;
    border: none;
    border-bottom: 1px solid %18;
}

/* ══ TOOLBAR ════════════════════════════════════════════════════════════════ */
QFrame#toolbar {
    background-color: %1;
    border: none;
    border-bottom: 1px solid %18;
}

/* ── Input panel ─────────────────────────────────────────────────────────── */
QFrame#inputPanel {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 6px;
}

/* ── Content area ────────────────────────────────────────────────────────── */
QStackedWidget#contentArea, QWidget#contentArea { background-color: %1; }

/* ══ CARDS ══════════════════════════════════════════════════════════════════ */
QFrame#card {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 6px;
}

/* ══ STAT BOXES ═════════════════════════════════════════════════════════════ */
QFrame#statBox {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 6px;
    min-width: 88px;
}
QFrame#statBox:hover { border-color: %14; background-color: %4; }

/* ══ SIDEBAR BUTTONS ════════════════════════════════════════════════════════ */
QPushButton#sideNav {
    background-color: transparent;
    color: %12;
    text-align: left;
    padding: 0px 12px 0px 14px;
    border-radius: 4px;
    border: none;
    border-left: 3px solid transparent;
    font-size: 13px;
    font-weight: 400;
}
QPushButton#sideNav:hover {
    background-color: %4;
    color: %12;
}
QPushButton#sideNav:checked {
    background-color: %4;
    color: %12;
    font-weight: 600;
    border-left: 3px solid %5;
}
QPushButton#sideNav:pressed { background-color: %15; }

/* Sidebar toggle */
QToolButton#sideToggleBtn {
    background-color: transparent;
    color: %12;
    border: none;
    border-radius: 4px;
    padding: 4px;
}
QToolButton#sideToggleBtn:hover   { background-color: %4; }
QToolButton#sideToggleBtn:pressed { background-color: %15; }

/* ══ BUTTONS ════════════════════════════════════════════════════════════════ */
QPushButton#btnAdd {
    background-color: %5;
    color: #ffffff;
    border: none;
    border-radius: 4px;
    padding: 0px 20px;
    font-size: 13px;
    font-weight: 600;
}
QPushButton#btnAdd:hover   { background-color: %6; }
QPushButton#btnAdd:pressed { background-color: %7; }
QPushButton#btnAdd:disabled { background-color: %21; color: %13; }

QPushButton#btnSecondary {
    background-color: %15;
    color: %12;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 400;
}
QPushButton#btnSecondary:hover   { background-color: %4; border-color: %14; }
QPushButton#btnSecondary:pressed { background-color: %16; }

QPushButton#btnDanger {
    background-color: %15;
    color: %10;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 400;
}
QPushButton#btnDanger:hover   { background-color: %4; }
QPushButton#btnDanger:pressed { background-color: %16; }

/* ── Tool buttons ────────────────────────────────────────────────────────── */
QToolButton#toolBtn {
    background-color: %15;
    color: %12;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 12px;
    font-weight: 400;
}
QToolButton#toolBtn:hover   { background-color: %4; border-color: %14; }
QToolButton#toolBtn:pressed { background-color: %16; }

QToolButton#toolBtnDanger {
    background-color: %15;
    color: %10;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 12px;
    font-weight: 400;
}
QToolButton#toolBtnDanger:hover { background-color: %4; }

/* ══ INPUTS ═════════════════════════════════════════════════════════════════ */
QLineEdit, QComboBox, QTimeEdit {
    background-color: %15;
    color: %12;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 5px 10px;
    font-size: 13px;
    selection-background-color: %5;
    selection-color: #ffffff;
}
QLineEdit:focus, QComboBox:focus, QTimeEdit:focus {
    border: 2px solid %5;
    background-color: %16;
}
QLineEdit:hover, QComboBox:hover, QTimeEdit:hover { border-color: %12; }
QLineEdit:disabled, QComboBox:disabled {
    background-color: %4;
    color: %21;
    border-color: %18;
}
QLineEdit#searchField {
    background-color: %15;
    border-radius: 4px;
    padding: 4px 12px;
    font-size: 12px;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: right center;
    width: 28px;
    border: none;
}
QComboBox::down-arrow { width: 12px; height: 12px; }
QComboBox QAbstractItemView {
    background-color: %3;
    border: 1px solid %14;
    border-radius: 4px;
    color: %12;
    selection-background-color: %4;
    selection-color: %12;
    padding: 2px;
}

/* ══ TABLES ═════════════════════════════════════════════════════════════════ */
QTableWidget {
    background-color: transparent;
    border: none;
    gridline-color: %18;
    color: %12;
    alternate-background-color: %2;
    selection-background-color: %4;
    selection-color: %12;
}
QTableWidget::item {
    padding: 6px 10px;
    border-bottom: 1px solid %18;
}
QTableWidget::item:selected { background-color: %4; color: %12; }
QTableWidget::item:hover    { background-color: %4; }
QHeaderView::section {
    background-color: %2;
    color: %13;
    padding: 7px 10px;
    border: none;
    border-bottom: 1px solid %18;
    font-size: 11px;
    font-weight: 600;
}
QHeaderView::section:hover { background-color: %4; color: %12; }

/* ── TextEdit (log view) ──────────────────────────────────────────────────── */
QTextEdit {
    background-color: %15;
    color: %12;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 8px 12px;
    font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace;
    font-size: 12px;
}
QTextEdit:focus { border: 2px solid %5; }

/* ══ PROGRESS BARS ══════════════════════════════════════════════════════════ */
QProgressBar {
    background-color: %4;
    border: none;
    border-radius: 2px;
    height: 4px;
    text-align: center;
    font-size: 10px;
    color: %12;
}
QProgressBar::chunk {
    background-color: %5;
    border-radius: 2px;
}

/* ══ CHECKBOXES ═════════════════════════════════════════════════════════════ */
QCheckBox { color: %12; font-size: 13px; spacing: 8px; }
QCheckBox::indicator {
    width: 18px; height: 18px;
    border: 1px solid %14;
    border-radius: 3px;
    background: %15;
}
QCheckBox::indicator:hover { border-color: %12; }
QCheckBox::indicator:checked {
    background: %5;
    border-color: %5;
}
QCheckBox::indicator:checked:hover { background: %6; border-color: %6; }

/* ══ LABELS ═════════════════════════════════════════════════════════════════ */
QLabel#sectionTitle {
    color: %12;
    font-size: 15px;
    font-weight: 600;
    background: transparent;
    border: none;
}
QLabel#sectionSub {
    color: %13;
    font-size: 12px;
    font-weight: 400;
    background: transparent;
    border: none;
}
QLabel#labelStat {
    color: %12;
    font-size: 18px;
    font-weight: 700;
    background: transparent;
    border: none;
}
QLabel#labelStatLabel {
    color: %13;
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 0.5px;
    background: transparent;
    border: none;
}

/* ══ STATUS BAR ═════════════════════════════════════════════════════════════ */
QStatusBar {
    background-color: %2;
    color: %13;
    border-top: 1px solid %18;
    font-size: 11px;
}
QStatusBar::item { border: none; }

/* ══ SCROLLBARS ═════════════════════════════════════════════════════════════ */
QScrollBar:vertical {
    background: transparent; width: 6px; margin: 2px 0px; border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: %14; border-radius: 3px; min-height: 24px;
}
QScrollBar::handle:vertical:hover   { background: %13; }
QScrollBar::handle:vertical:pressed { background: %5;  }
QScrollBar::add-line:vertical,  QScrollBar::sub-line:vertical  { height: 0; }
QScrollBar::add-page:vertical,  QScrollBar::sub-page:vertical  { background: transparent; }
QScrollBar:horizontal {
    background: transparent; height: 6px; margin: 0px 2px; border-radius: 3px;
}
QScrollBar::handle:horizontal {
    background: %14; border-radius: 3px; min-width: 24px;
}
QScrollBar::handle:horizontal:hover   { background: %13; }
QScrollBar::handle:horizontal:pressed { background: %5;  }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

/* ══ MENUS ══════════════════════════════════════════════════════════════════ */
QMenu {
    background: %3;
    border: 1px solid %18;
    border-radius: 6px;
    padding: 4px 2px;
    color: %12;
}
QMenu::item {
    padding: 7px 20px 7px 12px;
    border-radius: 3px;
    margin: 1px 3px;
    font-size: 13px;
    font-weight: 400;
}
QMenu::item:selected { background: %4; color: %12; }
QMenu::item:disabled { color: %21; }
QMenu::separator { height: 1px; background: %18; margin: 4px 8px; }
QMenu::icon { padding-left: 8px; }

/* ══ TOOLTIPS ═══════════════════════════════════════════════════════════════ */
QToolTip {
    background: %3;
    color: %12;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 5px 10px;
    font-size: 12px;
}
)")
    .arg(c.bg_primary)    // %1
    .arg(c.bg_secondary)  // %2
    .arg(c.card)          // %3
    .arg(c.hover)         // %4
    .arg(c.accent)        // %5
    .arg(c.accent_hover)  // %6
    .arg(c.accent2)       // %7
    .arg(c.success)       // %8
    .arg(c.warning)       // %9
    .arg(c.danger)        // %10
    .arg(c.text)          // %11 (duplicate slot, keeps numbering intact)
    .arg(c.text)          // %12 = text
    .arg(c.text_sec)      // %13 = text_sec
    .arg(c.border2)       // %14 = border2
    .arg(c.input_bg)      // %15 = input_bg
    .arg(c.input_focus)   // %16 = input_focus
    .arg(c.glow)          // %17 = glow
    .arg(c.border)        // %18 = border
    .arg(c.sidebar_top)   // %19 = sidebar_top
    .arg(c.sidebar_bot)   // %20 = sidebar_bot
    .arg(c.dim)           // %21 = dim
    ;
}
