#include "themeprovider.h"

bool ThemeProvider::s_isDark = true;

// ── IDM-Style Dark Palette ─────────────────────────────────────────────────
const Palette& ThemeProvider::dark() {
    static const Palette p = {
        .bg_primary   = u"#1c2028"_qs,
        .bg_secondary = u"#151920"_qs,
        .card         = u"#232830"_qs,
        .hover        = u"#2a3040"_qs,
        .accent       = u"#1a6fc4"_qs,
        .accent_hover = u"#2280d8"_qs,
        .accent2      = u"#4aa3f0"_qs,
        .success      = u"#3ea572"_qs,
        .warning      = u"#d4921a"_qs,
        .danger       = u"#cc4444"_qs,
        .text         = u"#e8eaf0"_qs,
        .text_sec     = u"#8892a8"_qs,
        .border       = u"#2c3244"_qs,
        .border2      = u"#38435a"_qs,
        .input_bg     = u"#1e2330"_qs,
        .input_focus  = u"#252d3e"_qs,
        .glow         = u"rgba(26,111,196,0.20)"_qs,
        .sidebar_top  = u"#1a1f2c"_qs,
        .sidebar_bot  = u"#141820"_qs,
        .dim          = u"#4e5a72"_qs,
    };
    return p;
}

// ── IDM-Style Light Palette ────────────────────────────────────────────────
const Palette& ThemeProvider::light() {
    static const Palette p = {
        .bg_primary   = u"#f0f2f5"_qs,
        .bg_secondary = u"#e4e7ec"_qs,
        .card         = u"#ffffff"_qs,
        .hover        = u"#dde3ec"_qs,
        .accent       = u"#1a6fc4"_qs,
        .accent_hover = u"#155da0"_qs,
        .accent2      = u"#0f4a82"_qs,
        .success      = u"#217a3c"_qs,
        .warning      = u"#7d4800"_qs,
        .danger       = u"#b52a2a"_qs,
        .text         = u"#1a1e28"_qs,
        .text_sec     = u"#5a6478"_qs,
        .border       = u"#ccd2de"_qs,
        .border2      = u"#a8b2c4"_qs,
        .input_bg     = u"#ffffff"_qs,
        .input_focus  = u"#f4f7fb"_qs,
        .glow         = u"rgba(26,111,196,0.12)"_qs,
        .sidebar_top  = u"#e8ecf4"_qs,
        .sidebar_bot  = u"#dde2ee"_qs,
        .dim          = u"#94a0b8"_qs,
    };
    return p;
}

const Palette& ThemeProvider::active() { return s_isDark ? dark() : light(); }
bool ThemeProvider::isDark() { return s_isDark; }
QString ThemeProvider::applyDark()  { s_isDark = true;  return buildStyleSheet(dark()); }
QString ThemeProvider::applyLight() { s_isDark = false; return buildStyleSheet(light()); }

QString ThemeProvider::buildStyleSheet(const Palette& c) {
    const QString sheet =
        QStringLiteral(
R"IDM(/* GLOBAL */
* { font-family: 'Segoe UI', Arial, sans-serif; font-size: 13px; outline: none; }
QMainWindow, QWidget { background-color: %1; color: %12; }
QFrame#rootFrame { background-color: %1; }

/* MENU BAR */
QMenuBar {
    background-color: %3; color: %12;
    border: none; border-bottom: 1px solid %18;
    padding: 1px 4px; font-size: 13px;
}
QMenuBar::item { background: transparent; padding: 5px 10px; border-radius: 3px; }
QMenuBar::item:selected { background-color: %5; color: #ffffff; }
QMenuBar::item:pressed  { background-color: %6; color: #ffffff; }
QMenu {
    background-color: %3; border: 1px solid %14;
    border-radius: 4px; padding: 4px 0px; color: %12;
}
QMenu::item { padding: 7px 32px 7px 20px; font-size: 12px; min-width: 160px; }
QMenu::item:selected  { background-color: %5; color: #ffffff; }
QMenu::item:disabled  { color: %21; }
QMenu::separator      { height: 1px; background: %18; margin: 4px 0px; }
QMenu::icon           { padding-left: 8px; }

/* SIDEBAR */
QFrame#sidebar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %19, stop:1 %20);
    border: none; border-right: 1px solid %18;
}

/* IDM MAIN TOOLBAR */
QToolBar#idmToolBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %3, stop:0.5 %4, stop:1 %2);
    border: none; border-bottom: 2px solid %18; padding: 3px 6px; spacing: 2px;
}
QToolBar#idmToolBar::separator { width: 1px; background: %14; margin: 6px 4px; }

QToolButton#idmToolBtn {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %4, stop:1 %15);
    color: %12; border: 1px solid %14; border-radius: 4px;
    padding: 6px 14px 4px 14px; font-size: 11px; font-weight: 500;
    min-width: 62px; min-height: 52px;
}
QToolButton#idmToolBtn:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %4, stop:1 %4);
    border-color: %5; color: %5;
}
QToolButton#idmToolBtn:pressed {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %2, stop:1 %15);
    border-color: %5;
}
QToolButton#idmToolBtn:disabled { color: %21; border-color: %18; background: %15; }

QToolButton#idmToolBtnDanger {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %4, stop:1 %15);
    color: %10; border: 1px solid %14; border-radius: 4px;
    padding: 6px 14px 4px 14px; font-size: 11px; font-weight: 500;
    min-width: 62px; min-height: 52px;
}
QToolButton#idmToolBtnDanger:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %4, stop:1 %4);
    border-color: %10;
}
QToolButton#idmToolBtnDanger:pressed { background: %2; border-color: %10; }

/* HEADER */
QFrame#header { background-color: %3; border: none; border-bottom: 1px solid %18; }

/* TOOLBAR STRIP */
QFrame#toolbar { background-color: %1; border: none; border-bottom: 1px solid %18; }

/* INPUT PANEL */
QFrame#inputPanel { background-color: %3; border: 1px solid %18; border-radius: 5px; }

/* CONTENT */
QStackedWidget#contentArea, QWidget#contentArea { background-color: %1; }

/* CARD */
QFrame#card { background-color: %3; border: 1px solid %18; border-radius: 4px; }

/* STAT BOXES */
QFrame#statBox { background-color: %3; border: 1px solid %18; border-radius: 4px; min-width: 88px; }
QFrame#statBox:hover { background-color: %4; border-color: %14; }

/* SIDEBAR BUTTONS */
QPushButton#sideNav {
    background-color: transparent; color: %13; text-align: left;
    padding: 0px 14px 0px 16px; border: none; border-left: 3px solid transparent;
    border-radius: 0px; font-size: 13px; font-weight: 400;
}
QPushButton#sideNav:hover {
    background-color: rgba(26,111,196,0.12); color: %12; border-left: 3px solid %5;
}
QPushButton#sideNav:checked {
    background-color: rgba(26,111,196,0.18); color: %12;
    font-weight: 600; border-left: 3px solid %5;
}
QPushButton#sideNav:pressed { background-color: rgba(26,111,196,0.28); }

QToolButton#sideToggleBtn {
    background-color: transparent; color: %13; border: none; border-radius: 3px; padding: 4px; font-size: 11px;
}
QToolButton#sideToggleBtn:hover   { background-color: %4; color: %12; }
QToolButton#sideToggleBtn:pressed { background-color: rgba(26,111,196,0.20); }

/* PRIMARY BUTTON */
QPushButton#btnAdd {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %6, stop:1 %5);
    color: #ffffff; border: 1px solid %5; border-radius: 4px;
    padding: 0px 20px; font-size: 13px; font-weight: 600;
}
QPushButton#btnAdd:hover   { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %7, stop:1 %6); }
QPushButton#btnAdd:pressed { background: %5; }
QPushButton#btnAdd:disabled { background-color: %18; color: %21; border-color: %18; }
)IDM")
        + QStringLiteral(
R"IDM2(
/* SECONDARY BUTTON */
QPushButton#btnSecondary {
    background-color: %15; color: %12; border: 1px solid %14;
    border-radius: 4px; padding: 4px 14px; font-size: 12px; font-weight: 500;
}
QPushButton#btnSecondary:hover   { background-color: %4; border-color: %5; }
QPushButton#btnSecondary:pressed { background-color: %16; }

/* DANGER BUTTON */
QPushButton#btnDanger {
    background-color: %15; color: %10; border: 1px solid %14;
    border-radius: 4px; padding: 4px 14px; font-size: 12px; font-weight: 500;
}
QPushButton#btnDanger:hover   { background-color: %4; border-color: %10; }
QPushButton#btnDanger:pressed { background-color: %16; }

/* TOOL BUTTONS */
QToolButton#toolBtn {
    background-color: %15; color: %12; border: 1px solid %14;
    border-radius: 4px; padding: 3px 12px; font-size: 12px; font-weight: 500;
}
QToolButton#toolBtn:hover   { background-color: %4; border-color: %5; color: %5; }
QToolButton#toolBtn:pressed { background-color: %16; }
QToolButton#toolBtn:checked { background-color: %5; color: #ffffff; border-color: %5; }

QToolButton#toolBtnDanger {
    background-color: %15; color: %10; border: 1px solid %14;
    border-radius: 4px; padding: 3px 12px; font-size: 12px; font-weight: 500;
}
QToolButton#toolBtnDanger:hover   { background-color: %4; border-color: %10; }
QToolButton#toolBtnDanger:pressed { background-color: %16; }

/* INPUTS */
QLineEdit, QComboBox, QTimeEdit {
    background-color: %15; color: %12; border: 1px solid %14;
    border-radius: 4px; padding: 5px 10px; font-size: 13px;
    selection-background-color: %5; selection-color: #ffffff;
}
QLineEdit:focus, QComboBox:focus, QTimeEdit:focus { border: 2px solid %5; background-color: %16; }
QLineEdit:hover, QComboBox:hover, QTimeEdit:hover { border-color: %12; }
QLineEdit:disabled, QComboBox:disabled { background-color: %2; color: %21; border-color: %18; }
QLineEdit::placeholder { color: %21; }
QLineEdit#searchField { border-radius: 4px; padding: 4px 10px; font-size: 12px; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: right center; width: 26px; border: none; border-left: 1px solid %14; }
QComboBox::down-arrow { width: 10px; height: 10px; }
QComboBox QAbstractItemView {
    background-color: %3; border: 1px solid %14; border-radius: 0px;
    color: %12; selection-background-color: %5; selection-color: #ffffff; padding: 2px; outline: none;
}

/* TABLE */
QTableWidget#downloadTable, QTableWidget {
    background-color: %3; border: 1px solid %18; border-radius: 4px;
    gridline-color: transparent; color: %12;
    alternate-background-color: %2;
    selection-background-color: rgba(26,111,196,0.18);
    selection-color: %12; font-size: 12px;
}
QTableWidget::item { padding: 5px 10px; border: none; border-bottom: 1px solid %18; }
QTableWidget::item:selected { background-color: rgba(26,111,196,0.18); color: %12; }
QTableWidget::item:hover    { background-color: %4; }
QHeaderView { background-color: %2; border: none; }
QHeaderView::section {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %4, stop:1 %2);
    color: %13; padding: 7px 10px; border: none;
    border-right: 1px solid %18; border-bottom: 2px solid %5;
    font-size: 11px; font-weight: 700; letter-spacing: 0.4px; text-transform: uppercase;
}
QHeaderView::section:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(26,111,196,0.15), stop:1 %4);
    color: %12;
}
QHeaderView::section:last { border-right: none; }

/* TEXT EDIT */
QTextEdit {
    background-color: %2; color: %12; border: 1px solid %18; border-radius: 4px;
    padding: 8px 12px;
    font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace;
    font-size: 12px;
}
QTextEdit:focus { border-color: %5; }

/* PROGRESS BAR */
QProgressBar {
    background-color: %18; border: none; border-radius: 3px;
    height: 6px; text-align: center; font-size: 10px; color: transparent;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %5, stop:1 %7);
    border-radius: 3px;
}

/* CHECKBOX */
QCheckBox { color: %12; font-size: 13px; spacing: 7px; }
QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %14; border-radius: 3px; background: %15; }
QCheckBox::indicator:hover       { border-color: %5; }
QCheckBox::indicator:checked     { background: %5; border-color: %5; }
QCheckBox::indicator:checked:hover { background: %6; border-color: %6; }

/* LABELS */
QLabel#sectionTitle  { color: %12; font-size: 14px; font-weight: 600; background: transparent; border: none; }
QLabel#sectionSub    { color: %13; font-size: 12px; background: transparent; border: none; }
QLabel#labelStat     { color: %12; font-size: 17px; font-weight: 700; background: transparent; border: none; }
QLabel#labelStatLabel { color: %13; font-size: 9px; font-weight: 600; letter-spacing: 0.6px; text-transform: uppercase; background: transparent; border: none; }

/* STATUS BAR */
QStatusBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %3, stop:1 %2);
    color: %13; border-top: 1px solid %18; font-size: 11px;
}
QStatusBar::item { border: none; }
QStatusBar QLabel { background: transparent; border: none; padding: 0 4px; font-size: 11px; color: %13; }

/* SCROLLBARS */
QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
QScrollBar::handle:vertical { background: %14; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover   { background: %13; }
QScrollBar::handle:vertical:pressed { background: %5; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }
QScrollBar::handle:horizontal { background: %14; border-radius: 4px; min-width: 30px; }
QScrollBar::handle:horizontal:hover   { background: %13; }
QScrollBar::handle:horizontal:pressed { background: %5; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }

/* TOOLTIP */
QToolTip { background-color: %3; color: %12; border: 1px solid %14; border-radius: 3px; padding: 5px 9px; font-size: 11px; }

/* SPLITTER */
QSplitter::handle { background-color: %18; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical   { height: 1px; }

/* SCROLL AREA */
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }
)IDM2")
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
    .arg(c.text)          // %11
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
