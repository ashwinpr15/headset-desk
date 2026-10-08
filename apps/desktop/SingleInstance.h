#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLockFile>
#include <memory>

class SingleInstance final : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(QString key, QObject* parent = nullptr);
    bool primary() const { return primary_; }
    bool forwarded() const { return forwarded_; }
    QString error() const { return error_; }
signals:
    void openRequested();
private:
    std::unique_ptr<QLockFile> lock_;
    QLocalServer server_;
    bool primary_{false};
    bool forwarded_{false};
    QString error_;
};
