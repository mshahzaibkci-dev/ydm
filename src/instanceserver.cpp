#include "instanceserver.h"
#include "constants.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QTimer>
#include <QEventLoop>

bool signalExistingInstance() {
    QTcpSocket sock;
    sock.connectToHost(QHostAddress::LocalHost, INSTANCE_PORT);
    if (!sock.waitForConnected(500)) return false;
    sock.write(INSTANCE_MAGIC);
    sock.flush();
    sock.waitForBytesWritten(500);
    sock.disconnectFromHost();
    return true;
}

InstanceServer::InstanceServer(QObject* windowRef, QObject* parent)
    : QThread(parent), m_windowRef(windowRef)
{}

void InstanceServer::stop() {
    if (m_server)
        QMetaObject::invokeMethod(m_server, "close", Qt::QueuedConnection);
    quit();
    wait(2000);
}

void InstanceServer::run() {
    QTcpServer server;
    m_server = &server;

    if (!server.listen(QHostAddress::LocalHost, INSTANCE_PORT)) {
        m_server = nullptr;
        return;  // Port busy — another instance owns it; just exit
    }

    QEventLoop loop;
    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* sock = server.nextPendingConnection();
            if (!sock) continue;
            sock->waitForReadyRead(500);
            QByteArray data = sock->readAll();
            sock->close();
            delete sock;

            if (data.contains(QByteArray(INSTANCE_MAGIC).trimmed())) {
                // Invoke bringToFront on the GUI thread
                QMetaObject::invokeMethod(m_windowRef, "_bringToFront",
                                          Qt::QueuedConnection);
            }
        }
    });

    QObject::connect(this, &QThread::finished, &loop, &QEventLoop::quit);
    loop.exec();
    server.close();
    m_server = nullptr;
}
