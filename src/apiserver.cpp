#include "apiserver.h"
#include "constants.h"
#include "utils.h"
#include "downloadmanager.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>

// ─────────────────────────────────────────────
//  Global token (set at startup)
// ─────────────────────────────────────────────
static QString g_apiToken;
void setGlobalApiToken(const QString& tok) { g_apiToken = tok; }

// ─────────────────────────────────────────────
//  APIRequestHandler
// ─────────────────────────────────────────────
APIRequestHandler::APIRequestHandler(QTcpSocket* socket,
                                     const QString& apiToken,
                                     APIBridge*     bridge,
                                     QObject*       managerRef,
                                     QObject*       parent)
    : QObject(parent)
    , m_socket(socket)
    , m_apiToken(apiToken)
    , m_bridge(bridge)
    , m_managerRef(managerRef)
{}

void APIRequestHandler::sendJson(int code, const QByteArray& body) {
    QString origin = m_headers.value(u"origin"_qs);
    QString statusLine;
    switch (code) {
        case 200: statusLine = u"200 OK"_qs; break;
        case 204: statusLine = u"204 No Content"_qs; break;
        case 400: statusLine = u"400 Bad Request"_qs; break;
        case 401: statusLine = u"401 Unauthorized"_qs; break;
        case 403: statusLine = u"403 Forbidden"_qs; break;
        case 404: statusLine = u"404 Not Found"_qs; break;
        case 411: statusLine = u"411 Length Required"_qs; break;
        case 413: statusLine = u"413 Request Entity Too Large"_qs; break;
        case 415: statusLine = u"415 Unsupported Media Type"_qs; break;
        case 422: statusLine = u"422 Unprocessable Entity"_qs; break;
        default:  statusLine = QString::number(code); break;
    }

    QByteArray resp;
    resp += QStringLiteral("HTTP/1.1 %1\r\n").arg(statusLine).toUtf8();
    resp += "Content-Type: application/json; charset=utf-8\r\n";
    resp += QStringLiteral("Content-Length: %1\r\n").arg(body.size()).toUtf8();
    if (!origin.isEmpty() && originAllowed(origin))
        resp += QStringLiteral("Access-Control-Allow-Origin: %1\r\n").arg(origin).toUtf8();
    resp += "Vary: Origin\r\n";
    resp += "X-Content-Type-Options: nosniff\r\n";
    resp += "Referrer-Policy: no-referrer\r\n";
    resp += "Cache-Control: no-store\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;

    m_socket->write(resp);
    m_socket->flush();
}

void APIRequestHandler::sendError(int code, const QString& message) {
    QJsonObject o;
    o[u"error"_qs] = message;
    sendJson(code, QJsonDocument(o).toJson(QJsonDocument::Compact));
}

bool APIRequestHandler::clientIsLocal() const {
    QHostAddress addr = m_socket->peerAddress();
    // Accept 127.0.0.1 and ::1
    return addr.isLoopback()
        || addr == QHostAddress(QStringLiteral("127.0.0.1"))
        || addr == QHostAddress(QStringLiteral("::1"))
        || addr == QHostAddress::LocalHost
        || addr == QHostAddress::LocalHostIPv6;
}

bool APIRequestHandler::hostHeaderOk() const {
    QString host = m_headers.value(u"host"_qs).toLower().trimmed();
    return host == QStringLiteral("127.0.0.1:%1").arg(API_PORT)
        || host == QStringLiteral("localhost:%1").arg(API_PORT);
}

static const QSet<QString> s_extensionSchemes = {
    u"chrome-extension"_qs, u"moz-extension"_qs, u"edge-extension"_qs
};

bool APIRequestHandler::originAllowed(const QString& origin) const {
    if (origin.isEmpty()) return true;
    QUrl u(origin.toLower().trimmed());
    QString scheme = u.scheme();
    if (s_extensionSchemes.contains(scheme))
        return !u.host().isEmpty();
    if (scheme == u"http"_qs) {
        QString host = u.host().toLower();
        int port = u.port(80);
        if ((host == u"127.0.0.1"_qs || host == u"localhost"_qs) && port == API_PORT)
            return true;
    }
    return false;
}

bool APIRequestHandler::originOkForStateChange() const {
    QString origin = m_headers.value(u"origin"_qs).toLower().trimmed();
    if (!origin.isEmpty()) {
        QUrl u(origin);
        if (s_extensionSchemes.contains(u.scheme()))
            return originAllowed(origin);
    }
    QString sfs = m_headers.value(u"sec-fetch-site"_qs).toLower();
    if (sfs == u"cross-site"_qs || sfs == u"same-site"_qs)
        return false;
    return originAllowed(origin);
}

bool APIRequestHandler::tokenOk() const {
    QString provided = m_headers.value(u"x-ydm-token"_qs);
    if (provided.isEmpty() || m_apiToken.isEmpty()) return false;
    // Constant-time compare
    return provided == m_apiToken;
}

void APIRequestHandler::handle() {
    // Read request line + headers (wait up to 5 s)
    if (!m_socket->waitForReadyRead(5000)) {
        m_socket->close();
        return;
    }

    QByteArray raw = m_socket->readAll();
    // Minimal HTTP parsing
    int headerEnd = raw.indexOf("\r\n\r\n");
    if (headerEnd < 0) headerEnd = raw.indexOf("\n\n");
    QByteArray headerPart = raw.left(headerEnd);
    QByteArray body       = (headerEnd >= 0)
                            ? raw.mid(headerEnd + (raw[headerEnd+1] == '\n' ? 2 : 4))
                            : QByteArray();

    QList<QByteArray> lines = headerPart.split('\n');
    if (lines.isEmpty()) { m_socket->close(); return; }

    // Request line
    QByteArray reqLine = lines.first().trimmed();
    QList<QByteArray> parts = reqLine.split(' ');
    if (parts.size() < 2) { m_socket->close(); return; }

    m_method = QString::fromUtf8(parts[0]).toUpper();
    m_path   = QString::fromUtf8(parts[1]).split('?').first();

    // Headers
    for (int i = 1; i < lines.size(); ++i) {
        QString line = QString::fromUtf8(lines[i]).trimmed();
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        m_headers.insert(line.left(colon).trimmed().toLower(),
                         line.mid(colon + 1).trimmed());
    }

    // If body was not fully received, try to read Content-Length bytes
    int contentLength = m_headers.value(u"content-length"_qs).toInt();
    if (contentLength > 0 && body.size() < contentLength) {
        while (body.size() < contentLength) {
            if (!m_socket->waitForReadyRead(3000)) break;
            body += m_socket->readAll();
        }
    }

    // Dispatch
    if (m_method == u"OPTIONS"_qs) {
        handleOptions();
    } else if (m_method == u"GET"_qs) {
        handleGet(m_path);
    } else if (m_method == u"POST"_qs) {
        handlePost(m_path, body);
    } else {
        sendError(405, u"Method not allowed"_qs);
    }

    m_socket->disconnectFromHost();
}

void APIRequestHandler::handleOptions() {
    if (!clientIsLocal() || !hostHeaderOk()) {
        sendError(403, u"Forbidden – local check failed"_qs); return;
    }
    QString origin = m_headers.value(u"origin"_qs).toLower().trimmed();
    if (origin.isEmpty() || !originOkForStateChange()) {
        sendError(403, u"Forbidden – origin check failed"_qs); return;
    }

    QByteArray resp;
    resp += "HTTP/1.1 204 No Content\r\n";
    resp += QStringLiteral("Access-Control-Allow-Origin: %1\r\n").arg(origin).toUtf8();
    resp += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    resp += "Access-Control-Allow-Headers: Content-Type, X-YDM-Token\r\n";
    resp += "Access-Control-Max-Age: 600\r\n";
    resp += "Vary: Origin\r\n";
    resp += "Content-Length: 0\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    m_socket->write(resp);
    m_socket->flush();
}

void APIRequestHandler::handleGet(const QString& path) {
    if (!clientIsLocal()) { sendError(403, u"Forbidden"_qs); return; }
    if (!hostHeaderOk())  { sendError(403, u"Forbidden"_qs); return; }

    if (path == u"/ping"_qs) {
        QJsonObject o;
        o[u"status"_qs]  = u"running"_qs;
        o[u"version"_qs] = u"2.0"_qs;
        sendJson(200, QJsonDocument(o).toJson(QJsonDocument::Compact));
        return;
    }

    if (!tokenOk()) { sendError(401, u"Unauthorized"_qs); return; }

    if (path == u"/status"_qs) {
        QJsonObject o;
        DownloadManager* mgr = qobject_cast<DownloadManager*>(m_managerRef);
        if (mgr) {
            int running = 0, queued = 0, paused = 0;
            for (const auto* item : std::as_const(mgr->queue)) {
                if (item->status == DownloadStatus::Running)  ++running;
                if (item->status == DownloadStatus::Queued)   ++queued;
                if (item->status == DownloadStatus::Paused)   ++paused;
            }
            o[u"running"_qs] = running;
            o[u"queued"_qs]  = queued;
            o[u"paused"_qs]  = paused;
            o[u"total"_qs]   = running + queued + paused;
        } else {
            o[u"running"_qs] = 0; o[u"queued"_qs] = 0;
            o[u"paused"_qs] = 0;  o[u"total"_qs]  = 0;
        }
        sendJson(200, QJsonDocument(o).toJson(QJsonDocument::Compact));
        return;
    }

    sendError(404, u"Not found"_qs);
}

void APIRequestHandler::handlePost(const QString& path, const QByteArray& body) {
    if (!clientIsLocal())          { sendError(403, u"Forbidden"_qs); return; }
    if (!hostHeaderOk())           { sendError(403, u"Forbidden"_qs); return; }
    if (!originOkForStateChange()) { sendError(403, u"Forbidden – origin"_qs); return; }
    if (!tokenOk())                { sendError(401, u"Unauthorized"_qs); return; }
    if (path != u"/download"_qs)   { sendError(404, u"Not found"_qs); return; }

    QString ctype = m_headers.value(u"content-type"_qs).split(';').first().trimmed().toLower();
    if (ctype != u"application/json"_qs) {
        sendError(415, u"Content-Type must be application/json"_qs); return;
    }

    int len = m_headers.value(u"content-length"_qs).toInt();
    if (len <= 0 || len > API_MAX_BODY) {
        sendError(413, u"Missing or oversized body"_qs); return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        sendError(400, u"Invalid JSON"_qs); return;
    }

    QJsonObject data = doc.object();
    QString url     = data[u"url"_qs].toString().trimmed();
    QString title   = data[u"title"_qs].toString().trimmed();
    QString quality = data[u"quality"_qs].toString().trimmed().toLower();

    if (!isSafeRemoteUrl(url)) {
        sendError(422, u"Invalid or disallowed URL"_qs); return;
    }

    static const QSet<QString> validQualities = {
        u"best"_qs, u"1080p"_qs, u"720p"_qs, u"480p"_qs,
        u"360p"_qs, u"audio"_qs, u"mp3"_qs, u"m4a"_qs, u"worst"_qs
    };
    if (!validQualities.contains(quality)) quality = u"best"_qs;

    // Strip control characters from title
    static QRegularExpression ctrlRe(u"[\\x00-\\x1f\\x7f]"_qs);
    title = title.replace(ctrlRe, {}).left(200);

    // Emit via bridge to GUI thread
    if (m_bridge)
        QMetaObject::invokeMethod(m_bridge, "downloadRequested",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, url),
                                  Q_ARG(QString, title),
                                  Q_ARG(QString, quality));

    QJsonObject resp;
    resp[u"status"_qs] = u"queued"_qs;
    sendJson(200, QJsonDocument(resp).toJson(QJsonDocument::Compact));
}

// ─────────────────────────────────────────────
//  APIServerThread
// ─────────────────────────────────────────────
APIServerThread::APIServerThread(APIBridge* bridge, QObject* parent)
    : QThread(parent), m_bridge(bridge)
{}

void APIServerThread::stop() {
    if (m_server) {
        QMetaObject::invokeMethod(m_server, "close", Qt::QueuedConnection);
    }
    quit();
    wait(3000);
}

void APIServerThread::run() {
    QTcpServer server;
    m_server = &server;

    if (!server.listen(QHostAddress(QLatin1StringView(API_HOST)), API_PORT)) {
        m_error = server.errorString();
        m_server = nullptr;
        return;
    }

    // Event loop for this thread
    QEventLoop loop;

    QObject::connect(&server, &QTcpServer::newConnection, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* sock = server.nextPendingConnection();
            if (!sock) continue;

            // Handle each connection synchronously in a local event loop
            // (each request is tiny, latency doesn't matter)
            QObject::connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);

            auto* handler = new APIRequestHandler(
                sock, g_apiToken, m_bridge, m_managerRef, &server);
            handler->handle();
            delete handler;
        }
    });

    QObject::connect(this, &QThread::finished, &loop, &QEventLoop::quit);
    loop.exec();

    server.close();
    m_server = nullptr;
}
