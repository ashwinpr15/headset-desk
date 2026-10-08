#pragma once
#include "headsetdesk/DeviceSession.h"
#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QVariantList>
#include <functional>

Q_DECLARE_METATYPE(headsetdesk::Snapshot)

// This object and its children move to one worker thread before any I/O.
class DeviceWorker final : public QObject {
    Q_OBJECT
public:
    explicit DeviceWorker(QString simulation);
public slots:
    void start();
    void connectSelected(int index);
    void disconnectDevice();
    void refresh(bool batteryOnly = false);
    void rescan();
    void enableControls(bool enabled);
    void setNoise(int mode, int level, int voice = -1);
    void setSpeakToChat(bool enabled);
    void setDsee(bool enabled);
    void setEqualizer(int bass, QVariantList bands);
    void shutdown();
signals:
    void devicesChanged(QStringList names, int selected);
    void snapshotChanged(headsetdesk::Snapshot state, bool fullRefresh);
    void busyChanged(bool busy);
    void messageChanged(QString message);
    void stopped();
private:
    void discover();
    void open(const headsetdesk::Candidate& target, bool automatic);
    void scheduleReconnect();
    void runCommand(const std::function<void()>& command, QString success);
    QString simulation_;
    std::vector<headsetdesk::Candidate> candidates_;
    std::optional<headsetdesk::Candidate> target_;
    std::unique_ptr<headsetdesk::DeviceSession> session_;
    QTimer* batteryTimer_;
    QTimer* reconnectTimer_;
    int attempts_{0};
    bool reconnectAllowed_{false};
    bool stopping_{false};
};
