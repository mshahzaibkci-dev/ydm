#include "widgets/sidebarbutton.h"

SidebarButton::SidebarButton(const QString& text, const QIcon& icon, QWidget* parent)
    : QPushButton(parent), m_text(text)
{
    setObjectName(u"sideNav"_qs);
    setCheckable(true);
    setAutoExclusive(false); // MainWindow manages mutual exclusion
    setFixedHeight(44);
    setMinimumWidth(0);
    setCursor(Qt::PointingHandCursor);
    setIcon(icon);
    setIconSize(QSize(18, 18));
    setText(text);
}

void SidebarButton::setExpanded(bool expanded) {
    setText(expanded ? m_text : {});
    setToolTip(expanded ? {} : m_text);
}
