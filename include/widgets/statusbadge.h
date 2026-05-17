#pragma once
#include "constants.h"
#include <QLabel>

// Pill-shaped status badge matching Python StatusBadge
class StatusBadge : public QLabel {
    Q_OBJECT
public:
    explicit StatusBadge(DownloadStatus status = DownloadStatus::Queued,
                         QWidget* parent = nullptr);
    void setStatus(DownloadStatus status);
    DownloadStatus status() const { return m_status; }

private:
    DownloadStatus m_status;
};
