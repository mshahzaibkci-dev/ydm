#include "themeprovider.h"

bool ThemeProvider::s_isDark = true;

// ── Professional Dark  ─────────────────────────────────────────────────────
//  IDM-class design: deep neutral backgrounds, crisp accent blue,
//  strong typographic hierarchy, clean borders — no gimmicks.
const Palette& ThemeProvider::dark() {
    static const Palette p = {
        .bg_primary   = u"#1a1a1a"_qs,           // true-dark app background
        .bg_secondary = u"#141414"_qs,            // deeper recessed areas
        .card         = u"#242424"_qs,            // surface / panel background
        .hover        = u"#2e2e2e"_qs,            // hover state fill
        .accent       = u"#0078d4"_qs,            // Windows accent blue
        .accent_hover = u"#1a8fe0"_qs,
        .accent2      = u"#4db8ff"_qs,            // lighter blue for contrast
        .success      = u"#4caf7d"_qs,
        .warning      = u"#e8a317"_qs,
        .danger       = u"#e05252"_qs,
        .text         = u"#f0f0f0"_qs,
        .text_sec     = u"#9a9a9a"_qs,
        .border       = u"#2f2f2f"_qs,
        .border2      = u"#404040"_qs,
        .input_bg     = u"#1f1f1f"_qs,
        .input_focus  = u"#262626"_qs,
        .glow         = u"rgba(0,120,212,0.18)"_qs,
        .sidebar_top  = u"#1e1e1e"_qs,
        .sidebar_bot  = u"#161616"_qs,
        .dim          = u"#555555"_qs,
    };
    return p;
}

// ── Professional Light  ────────────────────────────────────────────────────
const Palette& ThemeProvider::light() {
    static const Palette p = {
        .bg_primary   = u"#f5f5f5"_qs,
        .bg_secondary = u"#eeeeee"_qs,
        .card         = u"#ffffff"_qs,
        .hover        = u"#e8e8e8"_qs,
        .accent       = u"#0078d4"_qs,
        .accent_hover = u"#006cbe"_qs,
        .accent2      = u"#005fa3"_qs,
        .success      = u"#107c10"_qs,
        .warning      = u"#8a4f00"_qs,
        .danger       = u"#c42b1c"_qs,
        .text         = u"#1a1a1a"_qs,
        .text_sec     = u"#6a6a6a"_qs,
        .border       = u"#d8d8d8"_qs,
        .border2      = u"#b8b8b8"_qs,
        .input_bg     = u"#ffffff"_qs,
        .input_focus  = u"#f8f8f8"_qs,
        .glow         = u"rgba(0,120,212,0.12)"_qs,
        .sidebar_top  = u"#efefef"_qs,
        .sidebar_bot  = u"#e5e5e5"_qs,
        .dim          = u"#b0b0b0"_qs,
    };
    return p;
}

const Palette& ThemeProvider::active() {
    return s_isDark ? dark() : light();
}

bool ThemeProvider::isDark() { return s_isDark; }

QString ThemeProvider::applyDark()  { s_isDark = true;  return buildStyleSheet(dark()); }
QString ThemeProvider::applyLight() { s_isDark = false; return buildStyleSheet(light()); }

QString ThemeProvider::buildStyleSheet(const Palette& c) {
    // MSVC C2026: single string literal too large (>16 KB).
    // Split into three QStringLiteral chunks that are concatenated before .arg() expansion.
    const QString sheet =
        QStringLiteral(
R"(/* ═══════════════════════════════════════════════════════════════════════════
   GLOBAL
═══════════════════════════════════════════════════════════════════════════ */
* {
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 13px;
    outline: none;
}
QMainWindow, QWidget {
    background-color: %1;
    color: %12;
}
QFrame#rootFrame { background-color: %1; }

/* ═══════════════════════════════════════════════════════════════════════════
   SIDEBAR
   Slightly darker than the main canvas, clean top-to-bottom gradient,
   a single right border line — exactly like a professional app chrome.
═══════════════════════════════════════════════════════════════════════════ */
QFrame#sidebar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
        stop:0 %19, stop:1 %20);
    border: none;
    border-right: 1px solid %18;
}

/* ═══════════════════════════════════════════════════════════════════════════
   HEADER STRIP
   Same card surface, bottom border divides it from content cleanly.
═══════════════════════════════════════════════════════════════════════════ */
QFrame#header {
    background-color: %3;
    border: none;
    border-bottom: 1px solid %18;
}

/* ═══════════════════════════════════════════════════════════════════════════
   TOOLBAR STRIP
═══════════════════════════════════════════════════════════════════════════ */
QFrame#toolbar {
    background-color: %1;
    border: none;
    border-bottom: 1px solid %18;
}

/* ═══════════════════════════════════════════════════════════════════════════
   INPUT PANEL
   Elevated card with a stronger border so it reads as an action zone.
═══════════════════════════════════════════════════════════════════════════ */
QFrame#inputPanel {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 5px;
}

/* ═══════════════════════════════════════════════════════════════════════════
   CONTENT AREA
═══════════════════════════════════════════════════════════════════════════ */
QStackedWidget#contentArea, QWidget#contentArea {
    background-color: %1;
}

/* ═══════════════════════════════════════════════════════════════════════════
   GENERIC CARD
═══════════════════════════════════════════════════════════════════════════ */
QFrame#card {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 4px;
}

/* ═══════════════════════════════════════════════════════════════════════════
   STAT BOXES  (header counters: ACTIVE / QUEUED / DONE / SPEED)
   Clean rectangular tiles — no excessive rounding, strong accent top line.
═══════════════════════════════════════════════════════════════════════════ */
QFrame#statBox {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 4px;
    min-width: 88px;
}
QFrame#statBox:hover {
    background-color: %4;
    border-color: %14;
}

/* ═══════════════════════════════════════════════════════════════════════════
   SIDEBAR NAVIGATION BUTTONS
   Left accent stripe on active, subtle fill on hover — classic sidebar UX.
═══════════════════════════════════════════════════════════════════════════ */
QPushButton#sideNav {
    background-color: transparent;
    color: %13;
    text-align: left;
    padding: 0px 14px 0px 16px;
    border: none;
    border-left: 3px solid transparent;
    border-radius: 0px;
    font-size: 13px;
    font-weight: 400;
    letter-spacing: 0px;
}
QPushButton#sideNav:hover {
    background-color: %4;
    color: %12;
    border-left: 3px solid %14;
}
QPushButton#sideNav:checked {
    background-color: %4;
    color: %12;
    font-weight: 600;
    border-left: 3px solid %5;
}
QPushButton#sideNav:pressed {
    background-color: %15;
}

/* Sidebar collapse toggle */
QToolButton#sideToggleBtn {
    background-color: transparent;
    color: %13;
    border: none;
    border-radius: 3px;
    padding: 4px;
    font-size: 11px;
}
QToolButton#sideToggleBtn:hover   { background-color: %4; color: %12; }
QToolButton#sideToggleBtn:pressed { background-color: %15; }

/* ═══════════════════════════════════════════════════════════════════════════
   PRIMARY BUTTON  (+ Download)
   Solid accent fill, no gradient theatrics — professionals use flat buttons.
═══════════════════════════════════════════════════════════════════════════ */
QPushButton#btnAdd {
    background-color: %5;
    color: #ffffff;
    border: 1px solid %6;
    border-radius: 4px;
    padding: 0px 20px;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.2px;
}
QPushButton#btnAdd:hover   { background-color: %6; border-color: %6; }
QPushButton#btnAdd:pressed { background-color: %7; }
QPushButton#btnAdd:disabled {
    background-color: %18;
    color: %21;
    border-color: %18;
}

)")
        + QStringLiteral(
R"(/* ═══════════════════════════════════════════════════════════════════════════
   SECONDARY BUTTON
═══════════════════════════════════════════════════════════════════════════ */
QPushButton#btnSecondary {
    background-color: %15;
    color: %12;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 500;
}
QPushButton#btnSecondary:hover   { background-color: %4; border-color: %5; }
QPushButton#btnSecondary:pressed { background-color: %16; }

/* ═══════════════════════════════════════════════════════════════════════════
   DANGER BUTTON
═══════════════════════════════════════════════════════════════════════════ */
QPushButton#btnDanger {
    background-color: %15;
    color: %10;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 4px 14px;
    font-size: 12px;
    font-weight: 500;
}
QPushButton#btnDanger:hover   { background-color: %4; border-color: %10; }
QPushButton#btnDanger:pressed { background-color: %16; }

/* ═══════════════════════════════════════════════════════════════════════════
   TOOL BUTTONS  (toolbar actions)
═══════════════════════════════════════════════════════════════════════════ */
QToolButton#toolBtn {
    background-color: %15;
    color: %12;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 3px 12px;
    font-size: 12px;
    font-weight: 500;
}
QToolButton#toolBtn:hover   { background-color: %4; border-color: %5; color: %5; }
QToolButton#toolBtn:pressed { background-color: %16; }
QToolButton#toolBtn:checked {
    background-color: %5;
    color: #ffffff;
    border-color: %5;
}

QToolButton#toolBtnDanger {
    background-color: %15;
    color: %10;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 3px 12px;
    font-size: 12px;
    font-weight: 500;
}
QToolButton#toolBtnDanger:hover   { background-color: %4; border-color: %10; }
QToolButton#toolBtnDanger:pressed { background-color: %16; }

/* ═══════════════════════════════════════════════════════════════════════════
   INPUTS  (QLineEdit, QComboBox, QTimeEdit)
   Sharp focus ring matching the accent — industry standard UX pattern.
═══════════════════════════════════════════════════════════════════════════ */
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
QLineEdit:hover, QComboBox:hover, QTimeEdit:hover {
    border-color: %12;
}
QLineEdit:disabled, QComboBox:disabled {
    background-color: %2;
    color: %21;
    border-color: %18;
}
QLineEdit::placeholder {
    color: %21;
}
QLineEdit#searchField {
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 12px;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: right center;
    width: 26px;
    border: none;
    border-left: 1px solid %14;
}
QComboBox::down-arrow {
    width: 10px;
    height: 10px;
}
QComboBox QAbstractItemView {
    background-color: %3;
    border: 1px solid %14;
    border-radius: 0px;
    color: %12;
    selection-background-color: %5;
    selection-color: #ffffff;
    padding: 2px;
    outline: none;
}

/* ═══════════════════════════════════════════════════════════════════════════
   TABLE  (download list)
   Clean alternating rows, no grid lines, precise column headers.
═══════════════════════════════════════════════════════════════════════════ */
QTableWidget {
    background-color: %3;
    border: 1px solid %18;
    border-radius: 4px;
    gridline-color: transparent;
    color: %12;
    alternate-background-color: %2;
    selection-background-color: rgba(0,120,212,0.15);
    selection-color: %12;
}
QTableWidget::item {
    padding: 5px 10px;
    border: none;
    border-bottom: 1px solid %18;
}
QTableWidget::item:selected {
    background-color: rgba(0,120,212,0.15);
    color: %12;
}
QTableWidget::item:hover {
    background-color: %4;
}
QHeaderView {
    background-color: %2;
    border: none;
}
QHeaderView::section {
    background-color: %2;
    color: %13;
    padding: 6px 10px;
    border: none;
    border-right: 1px solid %18;
    border-bottom: 2px solid %18;
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 0.3px;
    text-transform: uppercase;
}
QHeaderView::section:hover {
    background-color: %4;
    color: %12;
}
QHeaderView::section:last {
    border-right: none;
}

/* ═══════════════════════════════════════════════════════════════════════════
   TEXT EDIT  (log view)
═══════════════════════════════════════════════════════════════════════════ */
QTextEdit {
    background-color: %2;
    color: %12;
    border: 1px solid %18;
    border-radius: 4px;
    padding: 8px 12px;
    font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace;
    font-size: 12px;
    line-height: 1.5;
}
QTextEdit:focus {
    border-color: %5;
}

/* ═══════════════════════════════════════════════════════════════════════════
   PROGRESS BAR
   Thin, clean, accent-filled — similar to browsers and IDM.
═══════════════════════════════════════════════════════════════════════════ */
QProgressBar {
    background-color: %18;
    border: none;
    border-radius: 3px;
    height: 6px;
    text-align: center;
    font-size: 10px;
    font-weight: 600;
    color: transparent;
}
QProgressBar::chunk {
    background-color: %5;
    border-radius: 3px;
}

)")
        + QStringLiteral(
R"(/* ═══════════════════════════════════════════════════════════════════════════
   CHECKBOX
═══════════════════════════════════════════════════════════════════════════ */
QCheckBox {
    color: %12;
    font-size: 13px;
    spacing: 7px;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid %14;
    border-radius: 3px;
    background: %15;
}
QCheckBox::indicator:hover       { border-color: %5; }
QCheckBox::indicator:checked     { background: %5; border-color: %5; }
QCheckBox::indicator:checked:hover { background: %6; border-color: %6; }

/* ═══════════════════════════════════════════════════════════════════════════
   LABELS
═══════════════════════════════════════════════════════════════════════════ */
QLabel#sectionTitle {
    color: %12;
    font-size: 14px;
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
    font-size: 17px;
    font-weight: 700;
    background: transparent;
    border: none;
}
QLabel#labelStatLabel {
    color: %13;
    font-size: 9px;
    font-weight: 600;
    letter-spacing: 0.6px;
    text-transform: uppercase;
    background: transparent;
    border: none;
}

/* ═══════════════════════════════════════════════════════════════════════════
   STATUS BAR
═══════════════════════════════════════════════════════════════════════════ */
QStatusBar {
    background-color: %3;
    color: %13;
    border-top: 1px solid %18;
    font-size: 11px;
}
QStatusBar::item { border: none; }

/* ═══════════════════════════════════════════════════════════════════════════
   SCROLLBARS
   Minimal, unobtrusive — visible only when needed.
═══════════════════════════════════════════════════════════════════════════ */
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: %14;
    border-radius: 4px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover   { background: %13; }
QScrollBar::handle:vertical:pressed { background: %5;  }
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical       { height: 0; }
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical       { background: transparent; }

QScrollBar:horizontal {
    background: transparent;
    height: 8px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: %14;
    border-radius: 4px;
    min-width: 30px;
}
QScrollBar::handle:horizontal:hover   { background: %13; }
QScrollBar::handle:horizontal:pressed { background: %5;  }
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal       { width: 0; }
QScrollBar::add-page:horizontal,
QScrollBar::sub-page:horizontal       { background: transparent; }

/* ═══════════════════════════════════════════════════════════════════════════
   CONTEXT MENUS
   Clean white/dark card, no rounded theatrics.
═══════════════════════════════════════════════════════════════════════════ */
QMenu {
    background-color: %3;
    border: 1px solid %14;
    border-radius: 4px;
    padding: 3px 0px;
    color: %12;
}
QMenu::item {
    padding: 6px 28px 6px 14px;
    border-radius: 0px;
    font-size: 12px;
    font-weight: 400;
}
QMenu::item:selected  { background-color: %5; color: #ffffff; }
QMenu::item:disabled  { color: %21; }
QMenu::separator      { height: 1px; background: %18; margin: 3px 0px; }
QMenu::icon           { padding-left: 8px; }

/* ═══════════════════════════════════════════════════════════════════════════
   TOOLTIPS
═══════════════════════════════════════════════════════════════════════════ */
QToolTip {
    background-color: %3;
    color: %12;
    border: 1px solid %14;
    border-radius: 3px;
    padding: 4px 8px;
    font-size: 11px;
}
)")
        ;
    return sheet
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
    .arg(c.text)          // %11 (dup — keeps slot numbering intact)
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
