#pragma once
#include <QLabel>

// Small flat chip for speed / ETA (Python: ChipLabel)
class StatChip : public QLabel {
    Q_OBJECT
public:
    explicit StatChip(const QString& text = {},
                      const QString& color = u"#9f97ff"_qs,
                      QWidget* parent = nullptr);
    void setColor(const QString& color);

private:
    void applyStyle();
    QString m_color;
};
