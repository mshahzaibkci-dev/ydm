#pragma once
#include <QPushButton>

// Checkable sidebar nav button
class SidebarButton : public QPushButton {
    Q_OBJECT
public:
    explicit SidebarButton(const QString& text,
                           const QIcon&   icon  = {},
                           QWidget*       parent = nullptr);
    void setExpanded(bool expanded);

private:
    QString m_text;
};
