#include "widgets/backgroundloader.h"
#include "persistence.h"

BackgroundLoader::BackgroundLoader(QObject* parent)
    : QThread(parent)
{}

void BackgroundLoader::run() {
    auto history = Persistence::loadHistory();
    emit historyReady(history);

    auto items = Persistence::loadQueue();
    emit queueReady(items);
}
