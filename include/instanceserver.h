#pragma once
#include <QThread>
#include <QTcpServer>

class QMainWindow;

// ─────────────────────────────────────────────
//  Single-instance signalling
//  Sends RAINAX_SHOW_WINDOW\n to an already-running instance.
//  Returns true if a running instance was found and notified.
// ─────────────────────────────────────────────
bool signalExistingInstance();

// ─────────────────────────────────────────────
//  Listens on INSTANCE_PORT for 'show window' pings
// ─────────────────────────────────────────────
class InstanceServer : public QThread {
    Q_OBJECT
public:
    explicit InstanceServer(QObject* windowRef, QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

private:
    QObject*    m_windowRef;
    QTcpServer* m_server = nullptr;
};
