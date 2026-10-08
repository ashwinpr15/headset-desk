#include "DeviceController.h"
#include <QDateTime>
#include <cmath>

using namespace headsetdesk;
namespace {
template<class T> QString errorOf(const Telemetry<T>& field) {
    return field.error.empty() ? "Not reported by the headphones." : QString::fromStdString(field.error);
}
}

DeviceController::DeviceController(QString simulation, QObject* parent) : QObject(parent),
    worker_(new DeviceWorker(simulation)), simulated_(!simulation.isEmpty()) {
    qRegisterMetaType<Snapshot>();
    apply({}, false);
    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::started, worker_, &DeviceWorker::start);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &DeviceWorker::stopped, &thread_, &QThread::quit, Qt::DirectConnection);
    connect(&thread_, &QThread::finished, this, &DeviceController::finished);
    connect(worker_, &DeviceWorker::snapshotChanged, this, &DeviceController::apply);
    connect(worker_, &DeviceWorker::devicesChanged, this, [this](QStringList names, int selected) {
        devices_ = std::move(names); selected_ = selected; emit changed();
    });
    connect(worker_, &DeviceWorker::busyChanged, this, [this](bool busy) { busy_ = busy || quitting_; emit changed(); });
    connect(worker_, &DeviceWorker::messageChanged, this, [this](QString text) { message_ = quitting_ ? "Closing the connection…" : text; emit changed(); });
    thread_.start();
}

DeviceController::~DeviceController() {
    if (thread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, &DeviceWorker::shutdown, Qt::QueuedConnection);
        thread_.wait(); // Bounded native I/O completes before worker/transport destruction.
    }
}

void DeviceController::apply(const Snapshot& snapshot, bool fullRefresh) {
    const bool connected = snapshot.connection == ConnectionState::Connected;
    const QString status = snapshot.connection == ConnectionState::Connecting ? "Connecting" :
        connected ? "Connected" : snapshot.connection == ConnectionState::Disconnected ? "Ready to connect" : "Disconnected";
    QString last = state_.value("updated", "—").toString();
    qint64 updatedEpoch = state_.value("updatedEpoch", 0).toLongLong();
    if (!connected) last = "—";
    else if (fullRefresh) last = QDateTime::currentDateTime().toString("h:mm AP");
    if (!connected) updatedEpoch = 0;
    else if (fullRefresh) updatedEpoch = QDateTime::currentMSecsSinceEpoch();
    QVariantList bands;
    for (int n = 0; n < 5; ++n) bands << (snapshot.equalizer.value ? QVariant(snapshot.equalizer.value->bands.at(n)) : QVariant{});
    state_ = {{"connected", connected}, {"status", status},
        {"controlsEnabled", connected && snapshot.controlsEnabled},
        {"canEnableControls", connected && snapshot.noise.value.has_value() && snapshot.equalizer.value.has_value() && snapshot.firmware.value.has_value()},
        {"model", snapshot.model ? QString::fromUtf8(profileFor(*snapshot.model).name.data()) : QString("Your headphones")},
        {"battery", snapshot.battery.value ? QString::number(*snapshot.battery.value) + "%" : "—"},
        {"batteryError", errorOf(snapshot.battery)},
        {"charging", snapshot.charging.value ? (*snapshot.charging.value ? "Charging" : "Not charging") : "—"},
        {"chargingError", errorOf(snapshot.charging)},
        {"firmware", snapshot.firmware.value ? QString::fromStdString(*snapshot.firmware.value) : "—"},
        {"firmwareError", errorOf(snapshot.firmware)},
        {"codec", snapshot.activeCodec.value ? QString::fromStdString(*snapshot.activeCodec.value) : "—"},
        {"codecError", errorOf(snapshot.activeCodec)},
        {"noiseKnown", snapshot.noise.value.has_value()}, {"noiseError", errorOf(snapshot.noise)},
        {"noiseMode", snapshot.noise.value ? static_cast<int>(snapshot.noise.value->mode) : -1},
        {"ambient", snapshot.noise.value ? snapshot.noise.value->ambientLevel : 1},
        {"voice", snapshot.noise.value ? snapshot.noise.value->focusOnVoice : false},
        {"speakKnown", snapshot.speakToChat.value.has_value()}, {"speak", snapshot.speakToChat.value.value_or(false)},
        {"speakError", errorOf(snapshot.speakToChat)},
        {"dseeKnown", snapshot.dsee.value.has_value()}, {"dsee", snapshot.dsee.value.value_or(false)},
        {"dseeError", errorOf(snapshot.dsee)},
        {"isXm5", snapshot.model && *snapshot.model == Model::Xm5},
        {"eqKnown", snapshot.equalizer.value.has_value()}, {"eqError", errorOf(snapshot.equalizer)},
        {"bands", bands}, {"bass", snapshot.equalizer.value ? QVariant(snapshot.equalizer.value->clearBass) : QVariant{}},
        {"lastChange", QString::fromStdString(snapshot.lastChange)},
        {"updated", last}, {"updatedEpoch", updatedEpoch}};
    emit changed();
}

void DeviceController::select(int index) {
    if (busy_ || quitting_ || index < 0 || index >= devices_.size() || selected_ == index) return;
    busy_ = true; selected_ = index; emit changed();
    QMetaObject::invokeMethod(worker_, &DeviceWorker::disconnectDevice, Qt::QueuedConnection);
}
void DeviceController::toggleConnection() {
    if (busy_ || quitting_) return;
    busy_ = true; emit changed();
    if (state_.value("connected").toBool()) QMetaObject::invokeMethod(worker_, &DeviceWorker::disconnectDevice, Qt::QueuedConnection);
    else QMetaObject::invokeMethod(worker_, [worker = worker_, index = selected_] { worker->connectSelected(index); }, Qt::QueuedConnection);
}
void DeviceController::refresh() {
    if (busy_ || quitting_ || !state_.value("connected").toBool()) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_] { worker->refresh(); }, Qt::QueuedConnection);
}
void DeviceController::rescan() {
    if (busy_ || quitting_ || state_.value("connected").toBool()) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, &DeviceWorker::rescan, Qt::QueuedConnection);
}
void DeviceController::quit() {
    if (quitting_) return;
    quitting_ = true; busy_ = true; message_ = "Closing the connection…"; emit changed();
    QMetaObject::invokeMethod(worker_, &DeviceWorker::shutdown, Qt::QueuedConnection);
}

void DeviceController::enableControls(bool enabled) {
    if (busy_ || quitting_ || !state_.value("connected").toBool() ||
        (enabled && !state_.value("canEnableControls").toBool())) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, enabled] { worker->enableControls(enabled); }, Qt::QueuedConnection);
}
void DeviceController::setNoise(int mode, int level) {
    if (busy_ || quitting_ || !state_.value("controlsEnabled").toBool() || mode < 0 || mode > 2 ||
        (level != -1 && (level < 1 || level > 20))) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, mode, level] { worker->setNoise(mode, level); }, Qt::QueuedConnection);
}
void DeviceController::setVoicePassthrough(bool enabled) {
    if (busy_ || quitting_ || !state_.value("controlsEnabled").toBool() || state_.value("noiseMode").toInt() != 2) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, enabled] { worker->setNoise(2, -1, enabled ? 1 : 0); }, Qt::QueuedConnection);
}
void DeviceController::setSpeakToChat(bool enabled) {
    if (busy_ || quitting_ || !state_.value("controlsEnabled").toBool() || !state_.value("speakKnown").toBool()) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, enabled] { worker->setSpeakToChat(enabled); }, Qt::QueuedConnection);
}
void DeviceController::setDsee(bool enabled) {
    if (busy_ || quitting_ || !state_.value("controlsEnabled").toBool() || !state_.value("dseeKnown").toBool()) return;
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, enabled] { worker->setDsee(enabled); }, Qt::QueuedConnection);
}
void DeviceController::applyEqualizer(int bass, QVariantList bands) {
    if (busy_ || quitting_ || !state_.value("controlsEnabled").toBool() || bass < -10 || bass > 10 || bands.size() != 5) return;
    for (const auto& value : bands) {
        bool ok = false; const double number = value.toDouble(&ok);
        if (!ok || !std::isfinite(number) || number != std::round(number) || number < -10 || number > 10) return;
    }
    busy_ = true; emit changed();
    QMetaObject::invokeMethod(worker_, [worker = worker_, bass, bands] { worker->setEqualizer(bass, bands); }, Qt::QueuedConnection);
}
