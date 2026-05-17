#pragma once
#include <QFrame>
#include <QLabel>

// Compact stat card (Python: StatBox)
class StatBox : public QFrame {
    Q_OBJECT
public:
    explicit StatBox(const QString& label,
                     const QString& value = u"0"_qs,
                     const QString& color = u"#7c6ef5"_qs,
                     QWidget* parent = nullptr);
    void setValue(const QString& v);

private:
    QLabel* m_labelLbl;
    QLabel* m_valueLbl;
    QString m_color;
};
