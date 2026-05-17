#include "ytdlpupdateworker.h"
#include "constants.h"
#include "utils.h"
#include <QProcess>

void YtdlpUpdateWorker::run() {
    emit progressMsg(u"Running pip install -U yt-dlp …"_qs);

    const QStringList& cmd = appPaths().ytdlpCmd;
    if (cmd.isEmpty()) {
        emit finishedErr(u"Could not locate Python interpreter."_qs);
        return;
    }

    QString python = cmd.first();
    // If ytdlpCmd is ["yt-dlp"], upgrade via pip separately
    QStringList args;
    if (cmd.size() == 1 && cmd.first().endsWith(u"yt-dlp"_qs)) {
        // standalone binary: notify user to update manually
        emit finishedErr(
            u"yt-dlp is installed as a standalone binary.\n"
            u"Run:  yt-dlp -U\nto update it."_qs);
        return;
    }
    // Python -m pip install -U yt-dlp
    args = { u"-m"_qs, u"pip"_qs, u"install"_qs, u"-U"_qs, u"yt-dlp"_qs };

    QProcess proc;
    proc.setProgram(python);
    proc.setArguments(args);
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start();

    if (!proc.waitForStarted(5000)) {
        emit finishedErr(u"Could not start pip process."_qs);
        return;
    }
    if (!proc.waitForFinished(120000)) {
        proc.kill();
        emit finishedErr(u"pip upgrade timed out after 120 s.\nTry manually: pip install -U yt-dlp"_qs);
        return;
    }

    if (proc.exitCode() != 0) {
        QString detail = QString::fromUtf8(proc.readAll()).trimmed().left(800);
        emit finishedErr(QStringLiteral("pip exited with code %1.\n\n%2")
                         .arg(proc.exitCode()).arg(detail));
        return;
    }

    emit finishedOk(getYtdlpVersion());
}
