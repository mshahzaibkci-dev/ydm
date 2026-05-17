#include "widgets/glossyprogressbar.h"
#include "themeprovider.h"
#include <QPainter>
#include <QLinearGradient>
#include <QEasingCurve>

GlossyProgressBar::GlossyProgressBar(QWidget* parent)
    : QWidget(parent), m_anim(this, "value")
{
    setMinimumHeight(20);
    setMaximumHeight(20);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(u"background: transparent; border: none;"_qs);

    m_anim.setDuration(320);
    m_anim.setEasingCurve(QEasingCurve::OutCubic);
}

void GlossyProgressBar::setValue(double v) {
    m_value = qBound(0.0, v, 100.0);
    update();
}

void GlossyProgressBar::setProgress(double pct) {
    pct = qBound(0.0, pct, 100.0);
    m_target = pct;
    m_anim.stop();
    m_anim.setStartValue(m_value);
    m_anim.setEndValue(pct);
    m_anim.start();
}

void GlossyProgressBar::setStatus(DownloadStatus s) {
    m_status = s;
    update();
}

void GlossyProgressBar::paintEvent(QPaintEvent*) {
    const Palette& c = ThemeProvider::active();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect rect = this->rect();
    int barH = 9;
    int barY = (rect.height() - barH) / 2;
    QRect barRect(rect.x(), barY, rect.width(), barH);

    // Track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(c.input_bg));
    p.drawRoundedRect(barRect, 5, 5);

    if (m_value > 0.001) {
        int fillW = std::max(1, int(barRect.width() * (m_value / 100.0)));
        QRect fillRect(barRect.x(), barRect.y(), fillW, barRect.height());

        QLinearGradient grad(fillRect.left(), 0, fillRect.right(), 0);
        switch (m_status) {
            case DownloadStatus::Completed:
                grad.setColorAt(0, QColor(c.success));
                grad.setColorAt(1, QColor(u"#1eeaa4"_qs));
                break;
            case DownloadStatus::Failed:
                grad.setColorAt(0, QColor(c.danger));
                grad.setColorAt(1, QColor(u"#f07090"_qs));
                break;
            case DownloadStatus::Paused:
                grad.setColorAt(0, QColor(c.warning));
                grad.setColorAt(1, QColor(u"#f8c060"_qs));
                break;
            case DownloadStatus::Cancelled:
                p.setBrush(QColor(c.border2));
                p.drawRoundedRect(fillRect, 5, 5);
                goto drawText;
            default:
                // Running / Starting – purple gradient
                grad.setColorAt(0.0,  QColor(c.accent));
                grad.setColorAt(0.55, QColor(c.accent2));
                grad.setColorAt(1.0,  QColor(c.accent));
                break;
        }
        p.setBrush(QBrush(grad));
        p.drawRoundedRect(fillRect, 5, 5);

        // Highlight sheen on top half
        if (m_value > 2) {
            QRect sheen(fillRect.x(), fillRect.y(), fillW, fillRect.height() / 2);
            QLinearGradient shineGrad(0, sheen.top(), 0, sheen.bottom());
            shineGrad.setColorAt(0, QColor(255, 255, 255, 42));
            shineGrad.setColorAt(1, QColor(255, 255, 255, 0));
            p.setBrush(QBrush(shineGrad));
            p.drawRoundedRect(sheen, 5, 5);
        }
    }

drawText:
    p.setPen(QColor(c.text));
    QFont f = p.font();
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect, Qt::AlignCenter,
               QStringLiteral("%1%").arg(static_cast<int>(m_value)));
    p.end();
}
