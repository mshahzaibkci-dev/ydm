#include "widgets/statchip.h"

StatChip::StatChip(const QString& text, const QString& color, QWidget* parent)
    : QLabel(text, parent), m_color(color)
{
    setAlignment(Qt::AlignCenter);
    setFixedHeight(20);
    applyStyle();
}

void StatChip::setColor(const QString& color) {
    m_color = color;
    applyStyle();
}

void StatChip::applyStyle() {
    setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background: rgba(124,110,245,0.12);"
        "  color: %1;"
        "  border: 1px solid rgba(124,110,245,0.22);"
        "  border-radius: 10px;"
        "  padding: 1px 10px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "  letter-spacing: 0.3px;"
        "}").arg(m_color));
}
