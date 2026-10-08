#include "SingleInstance.h"
#include <QLocalSocket>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QDeadlineTimer>

SingleInstance::SingleInstance(QString key, QObject* parent) : QObject(parent) {
    key += "-" + QString::number(qHash(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)), 16);
    const auto location = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/HeadsetDesk";
    QDir().mkpath(location);
    lock_ = std::make_unique<QLockFile>(location + "/" + key + ".lock");
    lock_->setStaleLockTime(0); // Dead process locks are still recovered by QLockFile.
    if (!lock_->tryLock(0)) {
        if (lock_->error() != QLockFile::LockFailedError) {
            error_ = "Couldn’t create the local instance lock."; return;
        }
        QLocalSocket socket;
        socket.connectToServer(key);
        if (socket.waitForConnected(3000)) {
            socket.write("OPEN\n");
            socket.waitForBytesWritten(1000);
            // The reply can arrive split or just after the server disconnects; read until
            // the full acknowledgement is in or the deadline passes.
            QByteArray reply;
            QDeadlineTimer deadline(2000);
            while (!reply.contains("OK\n") && !deadline.hasExpired()) {
                reply += socket.readAll();
                if (reply.contains("OK\n")) break;
                if (socket.state() != QLocalSocket::ConnectedState) break;
                socket.waitForReadyRead(static_cast<int>(qMax<qint64>(1, deadline.remainingTime())));
            }
            reply += socket.readAll();
            forwarded_ = reply.startsWith("OK\n");
        }
        if (!forwarded_) error_ = "Couldn’t reach the running instance.";
        return;
    }
    // Only the lock owner may remove a leftover endpoint.
    QLocalServer::removeServer(key);
    server_.setSocketOptions(QLocalServer::UserAccessOption);
    primary_ = server_.listen(key);
    if (!primary_) error_ = server_.errorString();
    connect(&server_, &QLocalServer::newConnection, this, [this] {
        while (auto* socket = server_.nextPendingConnection()) {
            socket->setParent(&server_);
            auto* deadline = new QTimer(socket);
            deadline->setSingleShot(true);
            deadline->start(2000);
            connect(deadline, &QTimer::timeout, socket, &QLocalSocket::abort);
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            auto* bytes = new QByteArray;
            connect(socket, &QObject::destroyed, this, [bytes] { delete bytes; });
            connect(socket, &QLocalSocket::readyRead, this, [this, socket, bytes] {
                *bytes += socket->readAll();
                if (*bytes == "OPEN\n") {
                    emit openRequested();
                    socket->write("OK\n"); socket->flush(); socket->disconnectFromServer();
                } else if (bytes->size() >= 5) socket->abort();
            });
        }
    });
}
