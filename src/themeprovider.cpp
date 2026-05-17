#include "themeprovider.h"

bool ThemeProvider::s_isDark = true;

const Palette& ThemeProvider::dark() {
    static const Palette p = {
        .bg_primary   = u"#06091a"_qs,
        .bg_secondary = u"#0a0f24"_qs,
        .card         = u"#0e1630"_qs,
        .hover        = u"#162040"_qs,
        .accent       = u"#7c6ef5"_qs,
        .accent_hover = u"#6355e0"_qs,
        .accent2      = u"#a89df8"_qs,
        .success      = u"#0fcf8a"_qs,
        .warning      = u"#f6a614"_qs,
        .danger       = u"#f4366a"_qs,
        .text         = u"#eef2ff"_qs,
        .text_sec     = u"#7a8fba"_qs,
        .border       = u"#1a2848"_qs,
        .border2      = u"#253860"_qs,
        .input_bg     = u"#0c1228"_qs,
        .input_focus  = u"#111830"_qs,
        .glow         = u"rgba(124,110,245,0.22)"_qs,
        .sidebar_top  = u"#0d1535"_qs,
        .sidebar_bot  = u"#060918"_qs,
        .dim          = u"#3d4f68"_qs,
    };
    return p;
}

const Palette& ThemeProvider::light() {
    static const Palette p = {
        .bg_primary   = u"#f0f2f8"_qs,
        .bg_secondary = u"#e8eaf2"_qs,
        .card         = u"#ffffff"_qs,
        .hover        = u"#e2e5f0"_qs,
        .accent       = u"#6355e0"_qs,
        .accent_hover = u"#4f42c8"_qs,
        .accent2      = u"#7c6ef5"_qs,
        .success      = u"#0a9e67"_qs,
        .warning      = u"#d4890a"_qs,
        .danger       = u"#d42060"_qs,
        .text         = u"#0f1630"_qs,
        .text_sec     = u"#4a5680"_qs,
        .border       = u"#c8cedf"_qs,
        .border2      = u"#a8b2cc"_qs,
        .input_bg     = u"#ffffff"_qs,
        .input_focus  = u"#f5f6fc"_qs,
        .glow         = u"rgba(99,85,224,0.15)"_qs,
        .sidebar_top  = u"#e6e9f5"_qs,
        .sidebar_bot  = u"#d8dced"_qs,
        .dim          = u"#8090b0"_qs,
    };
    return p;
}

const Palette& ThemeProvider::active() {
    return s_isDark ? dark() : light();
}

bool ThemeProvider::isDark() { return s_isDark; }

QString ThemeProvider::applyDark()  { s_isDark = true;  return buildStyleSheet(dark()); }
QString ThemeProvider::applyLight() { s_isDark = false; return buildStyleSheet(light()); }

// ─────────────────────────────────────────────
//  Full Qt Style Sheet  (1:1 with Python template)
// ─────────────────────────────────────────────
QString ThemeProvider::buildStyleSheet(const Palette& c) {
    return QStringLiteral(
R"(
/* ── Global ──────────────────────────────────── */
* {
    font-family: 'Segoe UI Variable', 'Segoe UI', 'Inter', Arial, sans-serif;
    font-size: 13px;
    outline: none;
}
QMainWindow, QWidget {
    background-color: %1;
    color: %12;
}
QFrame#rootFrame { background-color: %1; }

/* ══ SIDEBAR ══════════════════════════════════ */
QFrame#sidebar {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %19, stop:1 %20);
    border: none;
    border-right: 1px solid %13;
}

/* ══ HEADER ═══════════════════════════════════ */
QFrame#header {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 %3, stop:1 %2);
    border: none;
    border-bottom: 1px solid %13;
}

/* ══ TOOLBAR ══════════════════════════════════ */
QFrame#toolbar {
    background-color: %1;
    border: none;
    border-bottom: 1px solid %13;
}

/* ── Input panel ─────────────────────────────── */
QFrame#inputPanel {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
        stop:0 %3, stop:1 rgba(124,110,245,0.06));
    border: 1px solid %14;
    border-radius: 14px;
}

/* ── Content area ────────────────────────────── */
QStackedWidget#contentArea, QWidget#contentArea { background-color: %1; }

/* ══ CARDS ════════════════════════════════════ */
QFrame#card {
    background-color: %3;
    border: 1px solid %13;
    border-radius: 12px;
}

/* ══ STAT BOXES ═══════════════════════════════ */
QFrame#statBox {
    background-color: %3;
    border: 1px solid %13;
    border-radius: 10px;
    min-width: 88px;
}
QFrame#statBox:hover { border-color: %14; background-color: %4; }

/* ══ SIDEBAR BUTTONS ══════════════════════════ */
QPushButton#sideNav {
    background-color: transparent;
    color: %13;
    text-align: left;
    padding: 0px 12px 0px 14px;
    border-radius: 9px;
    border: none;
    border-left: 3px solid transparent;
    font-size: 13px;
    font-weight: 500;
    letter-spacing: 0.1px;
}
QPushButton#sideNav:hover {
    background-color: %4;
    color: %12;
    border-left: 3px solid %14;
}
QPushButton#sideNav:checked {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 rgba(124,110,245,0.18), stop:1 rgba(124,110,245,0.06));
    color: %7;
    font-weight: 600;
    border-left: 3px solid %5;
}
QPushButton#sideNav:pressed { background-color: rgba(124,110,245,0.24); }

/* Sidebar toggle */
QToolButton#sideToggleBtn {
    background-color: transparent;
    color: %13;
    border: none;
    border-radius: 7px;
    padding: 4px;
}
QToolButton#sideToggleBtn:hover { background-color: %4; color: %12; }
QToolButton#sideToggleBtn:pressed { background-color: %15; }

/* ══ BUTTONS ══════════════════════════════════ */
QPushButton#btnAdd {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %5, stop:0.5 %7, stop:1 %5);
    color: #ffffff;
    border: none;
    border-radius: 10px;
    padding: 0px 20px;
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.3px;
}
QPushButton#btnAdd:hover {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %6, stop:0.5 %5, stop:1 %6);
}
QPushButton#btnAdd:pressed { background-color: %6; }
QPushButton#btnAdd:disabled { background-color: %21; color: %13; }

QPushButton#btnSecondary {
    background-color: %16;
    color: %12;
    border: 1px solid %13;
    border-radius: 8px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 500;
}
QPushButton#btnSecondary:hover { background-color: %4; border-color: %14; }
QPushButton#btnSecondary:pressed { background-color: %3; }

QPushButton#btnDanger {
    background-color: rgba(244,54,106,0.12);
    color: %10;
    border: 1px solid rgba(244,54,106,0.3);
    border-radius: 8px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#btnDanger:hover { background-color: rgba(244,54,106,0.22); }
QPushButton#btnDanger:pressed { background-color: rgba(244,54,106,0.35); }

/* ── Tool buttons ─────────────────────────────── */
QToolButton#toolBtn {
    background-color: %16;
    color: %12;
    border: 1px solid %13;
    border-radius: 8px;
    padding: 4px 10px;
    font-size: 12px;
    font-weight: 500;
}
QToolButton#toolBtn:hover { background-color: %4; border-color: %14; }
QToolButton#toolBtn:pressed { background-color: %3; }

QToolButton#toolBtnDanger {
    background-color: rgba(244,54,106,0.10);
    color: %10;
    border: 1px solid rgba(244,54,106,0.25);
    border-radius: 8px;
    padding: 4px 10px;
    font-size: 12px;
    font-weight: 600;
}
QToolButton#toolBtnDanger:hover { background-color: rgba(244,54,106,0.20); }

/* ══ INPUTS ═══════════════════════════════════ */
QLineEdit, QComboBox, QTimeEdit {
    background-color: %15;
    color: %12;
    border: 1px solid %13;
    border-radius: 9px;
    padding: 6px 12px;
    font-size: 13px;
    selection-background-color: %5;
    selection-color: #ffffff;
}
QLineEdit:focus, QComboBox:focus, QTimeEdit:focus {
    border: 1.5px solid %5;
    background-color: %16;
    outline: none;
}
QLineEdit:hover, QComboBox:hover, QTimeEdit:hover { border-color: %14; }
QLineEdit:disabled, QComboBox:disabled {
    background-color: %3;
    color: %21;
    border-color: %13;
}
QLineEdit#searchField {
    background-color: %15;
    border-radius: 15px;
    padding: 4px 14px;
    font-size: 12px;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: right center;
    width: 28px;
    border: none;
}
QComboBox::down-arrow {
    width: 12px;
    height: 12px;
}
QComboBox QAbstractItemView {
    background-color: %3;
    border: 1px solid %14;
    border-radius: 8px;
    color: %12;
    selection-background-color: rgba(124,110,245,0.18);
    selection-color: %7;
    padding: 4px;
}

/* ══ TABLES ═══════════════════════════════════ */
QTableWidget {
    background-color: transparent;
    border: none;
    gridline-color: %13;
    color: %12;
    alternate-background-color: %2;
    selection-background-color: rgba(124,110,245,0.12);
    selection-color: %12;
}
QTableWidget::item {
    padding: 6px 10px;
    border-bottom: 1px solid %13;
}
QTableWidget::item:selected {
    background-color: rgba(124,110,245,0.15);
    color: %12;
}
QTableWidget::item:hover {
    background-color: rgba(124,110,245,0.08);
}
QHeaderView::section {
    background-color: %3;
    color: %13;
    padding: 8px 10px;
    border: none;
    border-bottom: 1px solid %13;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.5px;
    text-transform: uppercase;
}
QHeaderView::section:hover {
    background-color: %4;
    color: %12;
}

/* ── TextEdit (log view) ──────────────────────── */
QTextEdit {
    background-color: %15;
    color: %8;
    border: none;
    border-radius: 8px;
    padding: 10px 14px;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', 'Courier New', monospace;
    font-size: 12px;
}
QTextEdit:focus { border: 1px solid %5; }

/* ══ PROGRESS BARS ════════════════════════════ */
QProgressBar {
    background-color: %15;
    border: none;
    border-radius: 5px;
    height: 9px;
    text-align: center;
    font-size: 10px;
    font-weight: 700;
    color: %12;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 %5, stop:0.55 %7, stop:1 %5);
    border-radius: 5px;
}

/* ══ CHECKBOXES ═══════════════════════════════ */
QCheckBox { color: %12; font-size: 13px; spacing: 8px; }
QCheckBox::indicator {
    width: 18px; height: 18px;
    border: 1.5px solid %14;
    border-radius: 5px;
    background: %15;
}
QCheckBox::indicator:hover { border-color: %5; }
QCheckBox::indicator:checked {
    background: %5;
    border-color: %5;
}
QCheckBox::indicator:checked:hover { background: %6; border-color: %6; }

/* ══ LABELS ═══════════════════════════════════ */
QLabel#sectionTitle {
    color: %12;
    font-size: 15px;
    font-weight: 700;
    letter-spacing: 0.2px;
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
    font-weight: 800;
    background: transparent;
    border: none;
}
QLabel#labelStatLabel {
    color: %13;
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 0.8px;
    text-transform: uppercase;
    background: transparent;
    border: none;
}

/* ══ STATUS BAR ═══════════════════════════════ */
QStatusBar {
    background-color: %3;
    color: %13;
    border-top: 1px solid %13;
    font-size: 11px;
}
QStatusBar::item { border: none; }

/* ══ SCROLLBARS ═══════════════════════════════ */
QScrollBar:vertical {
    background: transparent; width: 5px; margin: 2px 0px; border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: %14; border-radius: 3px; min-height: 28px;
}
QScrollBar::handle:vertical:hover  { background: %5; }
QScrollBar::handle:vertical:pressed { background: %5; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal {
    background: transparent; height: 5px; margin: 0px 2px; border-radius: 3px;
}
QScrollBar::handle:horizontal {
    background: %14; border-radius: 3px; min-width: 28px;
}
QScrollBar::handle:horizontal:hover  { background: %5; }
QScrollBar::handle:horizontal:pressed { background: %5; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

/* ══ MENUS ════════════════════════════════════ */
QMenu {
    background: %3;
    border: 1px solid %14;
    border-radius: 10px;
    padding: 6px 4px;
    color: %12;
}
QMenu::item {
    padding: 8px 24px 8px 14px;
    border-radius: 7px;
    margin: 1px 4px;
    font-size: 12px;
    font-weight: 500;
}
QMenu::item:selected { background: rgba(124,110,245,0.18); color: %7; }
QMenu::item:disabled { color: %21; }
QMenu::separator { height: 1px; background: %13; margin: 6px 10px; }
QMenu::icon { padding-left: 8px; }

/* ══ TOOLTIPS ════════════════════════════════ */
QToolTip {
    background: %3;
    color: %12;
    border: 1px solid %14;
    border-radius: 8px;
    padding: 7px 14px;
    font-size: 12px;
    font-weight: 500;
}
)")
    // Positional args matching the palette fields:
    // %1=bg_primary  %2=bg_secondary  %3=card         %4=hover
    // %5=accent      %6=accent_hover  %7=accent2      %8=success
    // %9=warning     %10=danger       %11=(unused)     %12=text
    // %13=text_sec   (reused for border-like text_sec, see Python)
    // NOTE: In Python the stylesheet uses named keys, here we remap carefully.
    // We'll use the actual color values directly to avoid confusion:
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
    .arg(c.text)          // %11 (unused slot, keep for numbering)
    .arg(c.text)          // %12 = text
    .arg(c.text_sec)      // %13 = text_sec (used for borders, dim labels)
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
