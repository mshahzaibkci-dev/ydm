#pragma once
#include <QThread>

// Downloads a pre-built Windows ffmpeg.exe from yt-dlp/FFmpeg-Builds
// (Python: FfmpegUpdateWorker)
class FfmpegUpdateWorker : public QThread {
    Q_OBJECT
public:
    using QThread::QThread;
signals:
    void progressMsg(const QString& msg);
    void finishedOk(const QString& newVersion);
    void finishedErr(const QString& error);
protected:
    void run() override;

private:
    static const char* RELEASES_URL;
    static constexpr qint64 MAX_BYTES = 150LL * 1024 * 1024;
};
