#include "widgets/statbox.h"
#include <QVBoxLayout>

StatBox::StatBox(const QString& label, const QString& value,
                 const QString& color, QWidget* parent)
    : QFrame(parent), m_color(color)
{
    setObjectName(u"statBox"_qs);
    setFixedHeight(58);
    setMinimumWidth(88);
    setMaximumWidth(148);
    // Coloured top accent border
    setStyleSheet(QStringLiteral(
        "QFrame#statBox { border-top: 2px solid %1; "
        "border-top-left-radius: 10px; border-top-right-radius: 10px; }"
    ).arg(color));

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 8, 14, 8);
    lay->setSpacing(3);

    m_labelLbl = new QLabel(label.toUpper(), this);
    m_labelLbl->setObjectName(u"labelStatLabel"_qs);
    m_labelLbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    m_valueLbl = new QLabel(value, this);
    m_valueLbl->setObjectName(u"labelStat"_qs);
    m_valueLbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    lay->addWidget(m_labelLbl);
    lay->addWidget(m_valueLbl);
}

void StatBox::setValue(const QString& v) {
    m_valueLbl->setText(v);
}
