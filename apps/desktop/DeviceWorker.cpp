#include "DeviceWorker.h"
#include "headsetdesk/Simulation.h"
#include "sony/transport/PlatformTransport.h"
#include <QSettings>
#include <algorithm>

using namespace headsetdesk;

DeviceWorker::DeviceWorker(QString simulation) : simulation_(std::move(simulation)),
    batteryTimer_(new QTimer(this)), reconnectTimer_(new QTimer(this)) {
    batteryTimer_->setInterval(180000);
    reconnectTimer_->setSingleShot(true);
    connect(batteryTimer_, &QTimer::timeout, this, [this] { refresh(true); });
    connect(reconnectTimer_, &QTimer::timeout, this, [this] {
        if (reconnectAllowed_ && target_ && !stopping_) open(*target_, true);
    });
}

void DeviceWorker::discover() {
    if (!simulation_.isEmpty()) {
        candidates_ = {simulatedCandidate(Model::Xm5), simulatedCandidate(Model::Ch720n)};
    } else {
        auto discovery = sony::transport::createPlatformDiscovery();
        candidates_ = supportedPairedDevices(*discovery);
    }
    QSettings settings;
    auto preferred = settings.value("preferredDevice").toString().toStdString();
    auto automatic = startupTarget(candidates_, preferred);
    int selected = 0;
    QStringList names;
    for (std::size_t n = 0; n < candidates_.size(); ++n) {
        const auto& candidate = candidates_[n];
        names << QString::fromUtf8(profileFor(candidate.model).name.data());
        if ((automatic && automatic->device.address == candidate.device.address) ||
            (simulation_ == "ch720n" && candidate.model == Model::Ch720n)) selected = static_cast<int>(n);
    }
    emit devicesChanged(names, selected);
}

void DeviceWorker::start() {
    emit busyChanged(true);
    try {
        discover();
        QSettings settings;
        auto automatic = startupTarget(candidates_, settings.value("preferredDevice").toString().toStdString());
        if (simulation_ == "ch720n") automatic = candidates_.at(1);
        if (simulation_ == "idle") automatic.reset();
        if (automatic) { attempts_ = 0; open(*automatic, true); return; }
        else emit messageChanged(candidates_.empty() ? "Pair your headphones in Windows Bluetooth settings, then refresh the list." : "Ready when you are. Turn on your headphones and connect.");
    } catch (...) {
        emit messageChanged("Couldn’t read the paired headphones. Check Windows Bluetooth and refresh the list.");
    }
    emit busyChanged(false);
}

void DeviceWorker::open(const Candidate& target, bool automatic) {
    if (stopping_) return;
    batteryTimer_->stop();
    reconnectTimer_->stop();
    target_ = target;
    reconnectAllowed_ = automatic;
    emit busyChanged(true);
    Snapshot connecting; connecting.connection = ConnectionState::Connecting;
    connecting.model = target.model;
    emit snapshotChanged(connecting, false);
    emit messageChanged("Connecting…");
    try {
        session_.reset();
        session_ = std::make_unique<DeviceSession>(simulation_.isEmpty()
            ? makeWindowsReadOnlyTransport() : makeSimulatedTransport(target.model));
        session_->connect(target);
        session_->refresh();
        const auto snapshot = session_->state();
        emit snapshotChanged(snapshot, true);
        if (snapshot.connection == ConnectionState::Connected) {
            reconnectAllowed_ = true; attempts_ = 0; batteryTimer_->start();
            if (simulation_.isEmpty()) {
                QSettings settings;
                settings.setValue("preferredDevice", QString::fromStdString(target.device.address.str()));
            }
            emit messageChanged({});
        } else { reconnectAllowed_ = automatic; scheduleReconnect(); }
    } catch (...) {
        if (session_) emit snapshotChanged(session_->state(), false);
        emit messageChanged("Couldn’t connect. Turn on your headphones and check Windows Bluetooth.");
        scheduleReconnect();
    }
    emit busyChanged(false);
}

void DeviceWorker::connectSelected(int index) {
    attempts_ = 0; reconnectAllowed_ = false; reconnectTimer_->stop();
    if (index < 0 || index >= static_cast<int>(candidates_.size()) || stopping_) {
        emit busyChanged(false); return;
    }
    open(candidates_[index], false);
}

void DeviceWorker::disconnectDevice() {
    reconnectAllowed_ = false; attempts_ = 0;
    reconnectTimer_->stop(); batteryTimer_->stop();
    if (session_) session_->disconnect();
    session_.reset(); target_.reset();
    emit snapshotChanged({}, false);
    emit messageChanged("Disconnected. Connect whenever you’re ready.");
    emit busyChanged(false);
}

void DeviceWorker::refresh(bool batteryOnly) {
    if (!session_ || stopping_) { emit busyChanged(false); return; }
    emit busyChanged(true);
    if (batteryOnly) session_->refreshBattery(); else session_->refresh();
    const auto snapshot = session_->state();
    emit snapshotChanged(snapshot, !batteryOnly);
    if (snapshot.connection != ConnectionState::Connected) {
        batteryTimer_->stop();
        emit messageChanged("Connection lost. Checking again shortly…");
        scheduleReconnect();
    }
    emit busyChanged(false);
}

void DeviceWorker::scheduleReconnect() {
    if (!reconnectAllowed_ || stopping_ || !target_) return;
    if (attempts_ >= 5) {
        reconnectAllowed_ = false;
        emit messageChanged("Couldn’t reconnect. Turn on your headphones, then choose Connect.");
        return;
    }
    reconnectTimer_->start(std::min(30000, 1000 * (1 << attempts_++)));
}

void DeviceWorker::runCommand(const std::function<void()>& command, QString success) {
    if (!session_ || stopping_) { emit busyChanged(false); return; }
    emit busyChanged(true);
    try {
        command();
        emit messageChanged(std::move(success));
    } catch (const std::exception&) {
        session_->enableControls(false);
        session_->refresh(); // Show actual state after an uncertain write; never replay it.
        emit messageChanged("Couldn’t confirm the change. Controls disabled; check the readings before trying again.");
    }
    const auto snapshot = session_->state();
    emit snapshotChanged(snapshot, false);
    if (snapshot.connection != ConnectionState::Connected) {
        batteryTimer_->stop();
        scheduleReconnect();
    }
    emit busyChanged(false);
}

void DeviceWorker::enableControls(bool enabled) {
    runCommand([this, enabled] {
        if (enabled) session_->refresh();
        session_->enableControls(enabled);
    }, enabled ? "Experimental controls enabled for this connection." : QString{});
}

void DeviceWorker::setNoise(int mode, int level) {
    runCommand([this, mode, level] {
        session_->setNoise(static_cast<sony::protocol::NoiseControlMode>(mode), level);
    }, "Noise setting confirmed by headphones.");
}

void DeviceWorker::setEqualizer(int bass, QVariantList values) {
    std::vector<int> bands;
    for (const auto& value : values) bands.push_back(value.toInt());
    runCommand([this, bass, bands] { session_->setEqualizer(bass, bands); }, "EQ confirmed by headphones.");
}

void DeviceWorker::rescan() {
    if (stopping_) return;
    emit busyChanged(true);
    try { discover(); emit messageChanged(candidates_.empty() ? "No supported paired headphones found." : QString{}); }
    catch (...) { emit messageChanged("Couldn’t refresh the list. Check Windows Bluetooth."); }
    emit busyChanged(false);
}

void DeviceWorker::shutdown() {
    stopping_ = true;
    disconnectDevice();
    emit stopped();
}
