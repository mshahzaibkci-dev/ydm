#include "downloaditem.h"
#include "constants.h"
#include "utils.h"
#include <QDir>
#include <QUuid>

DownloadItem DownloadItem::create(const QString& url,
                                  const QString& outputDir,
                                  const QString& fmtKey)
{
    DownloadItem it;
    it.id        = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    it.url       = url;
    it.outputDir = QDir(outputDir).canonicalPath();
    if (it.outputDir.isEmpty()) it.outputDir = QDir(outputDir).absolutePath();
    it.fmtKey    = isValidFormatKey(fmtKey) ? fmtKey : u"Best Quality (auto)"_qs;
    it.fmtValue  = formatValueForKey(it.fmtKey);
    it.status    = DownloadStatus::Queued;
    it.addedAt   = QDateTime::currentDateTime().toString(u"hh:mm:ss"_qs);
    it.isDirect  = isDirectFileUrl(url);

    if (shouldUseYtdlp(url))
        it.displayFmt = it.fmtKey;
    else
        it.displayFmt = getGenericFileLabel(url);

    return it;
}
