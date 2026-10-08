#pragma once
#include "DeviceWorker.h"
#include <QThread>
#include <QVariantMap>

class DeviceController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QStringList devices READ devices NOTIFY changed)
    Q_PROPERTY(int selected READ selected NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool simulated READ simulated CONSTANT)
public:
    explicit DeviceController(QString simulation, QObject* parent = nullptr);
    ~DeviceController() override;
    QVariantMap state() const { return state_; }
    QStringList devices() const { return devices_; }
    int selected() const { return selected_; }
    bool busy() const { return busy_; }
    QString message() const { return message_; }
    bool simulated() const { return simulated_; }
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void toggleConnection();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void quit();
signals:
    void changed();
    void finished();
private:
    void apply(const headsetdesk::Snapshot& snapshot, bool fullRefresh);
    QThread thread_;
    DeviceWorker* worker_;
    QVariantMap state_;
    QStringList devices_;
    QString message_;
    int selected_{0};
    bool busy_{true};
    bool simulated_;
    bool quitting_{false};
};
