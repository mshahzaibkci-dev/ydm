#pragma once
#include "constants.h"
#include <QWidget>
#include <QPropertyAnimation>

// Matches Python GlossyProgressBar exactly:
// – 9 px tall filled track, rounded, gradient fill
// – Animated value via QPropertyAnimation
// – Percentage text centred
class GlossyProgressBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
public:
    explicit GlossyProgressBar(QWidget* parent = nullptr);

    double value() const  { return m_value; }
    void   setValue(double v);
    void   setProgress(double pct);
    void   setStatus(DownloadStatus s);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    double             m_value  = 0.0;
    double             m_target = 0.0;
    DownloadStatus     m_status = DownloadStatus::Queued;
    QPropertyAnimation m_anim;
};
