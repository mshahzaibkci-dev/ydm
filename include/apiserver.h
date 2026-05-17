#pragma once
#include <QObject>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

// ─────────────────────────────────────────────
//  Signal bridge (lives on GUI thread)
// ─────────────────────────────────────────────
class APIBridge : public QObject {
    Q_OBJECT
public:
    explicit APIBridge(QObject* parent = nullptr) : QObject(parent) {}
signals:
    void downloadRequested(const QString& url,
                           const QString& title,
                           const QString& quality);
};

// ─────────────────────────────────────────────
//  HTTP request handler (runs on server thread)
// ─────────────────────────────────────────────
class APIRequestHandler : public QObject {
    Q_OBJECT
public:
    explicit APIRequestHandler(QTcpSocket* socket,
                               const QString& apiToken,
                               APIBridge*     bridge,
                               QObject*       managerRef,
                               QObject*       parent = nullptr);
    void handle();

signals:
    void downloadRequested(const QString& url,
                           const QString& title,
                           const QString& quality);

private:
    void sendJson(int code, const QByteArray& body);
    void sendError(int code, const QString& message);
    bool clientIsLocal() const;
    bool hostHeaderOk() const;
    bool originAllowed(const QString& origin) const;
    bool originOkForStateChange() const;
    bool tokenOk() const;

    void handleGet(const QString& path);
    void handlePost(const QString& path,
                    const QByteArray& body);
    void handleOptions();

    QTcpSocket*  m_socket;
    QString      m_apiToken;
    APIBridge*   m_bridge;
    QObject*     m_managerRef;

    // Parsed request
    QString      m_method;
    QString      m_path;
    QMap<QString,QString> m_headers;
};

// ─────────────────────────────────────────────
//  TCP server (runs on its own thread)
// ─────────────────────────────────────────────
class APIServerThread : public QThread {
    Q_OBJECT
public:
    explicit APIServerThread(APIBridge* bridge, QObject* parent = nullptr);
    void stop();

    QString errorString() const { return m_error; }
    void setManagerRef(QObject* ref) { m_managerRef = ref; }

protected:
    void run() override;

private:
    APIBridge* m_bridge;
    QObject*   m_managerRef = nullptr;
    QString    m_error;
    QTcpServer* m_server = nullptr;
};
