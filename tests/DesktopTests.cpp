#include "DeviceController.h"
#include "SingleInstance.h"
#include <QtTest>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QProcess>
#include <QDir>
#include <QStyleHints>

class DesktopTests : public QObject {
    Q_OBJECT
private slots:
    void connectionSwitchingAndManualDisconnect() {
        DeviceController device("xm5");
        QTRY_VERIFY_WITH_TIMEOUT(!device.busy(), 10000);
        QCOMPARE(device.devices().size(), 2);
        QCOMPARE(device.selected(), 0);
        QCOMPARE(device.state().value("model").toString(), "WH-1000XM5");
        QVERIFY(device.state().value("connected").toBool());
        QCOMPARE(device.state().value("battery").toString(), "80%");
        device.select(1);
        QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("connected").toBool());
        QCOMPARE(device.state().value("battery").toString(), "—");
        device.toggleConnection();
        QTRY_VERIFY_WITH_TIMEOUT(!device.busy(), 10000);
        QCOMPARE(device.state().value("model").toString(), "WH-CH720N");
        QCOMPARE(device.state().value("firmware").toString(), "1.1.4");
        device.toggleConnection();
        QTRY_VERIFY(!device.busy());
        QTest::qWait(1500);
        QVERIFY(!device.state().value("connected").toBool());
        QCOMPARE(device.state().value("codec").toString(), "—");
        QSignalSpy finished(&device, &DeviceController::finished);
        device.quit();
        QTRY_COMPARE(finished.count(), 1);
    }
    void idleAndRefreshList() {
        DeviceController device("idle");
        QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("connected").toBool());
        QCOMPARE(device.state().value("battery").toString(), "—");
        device.rescan(); QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("connected").toBool());
        device.toggleConnection(); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("connected").toBool());
        device.refresh(); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("battery").toString(), "80%");
    }
    void windowCloseKeepsSessionAndReadOnlyControls() {
        DeviceController device("ch720n");
        QTRY_VERIFY(!device.busy());
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("device", &device);
        engine.rootContext()->setContextProperty("darkTheme", false);
        engine.rootContext()->setContextProperty("trayAvailable", true);
        engine.rootContext()->setContextProperty("reduceMotion", true);
        engine.load(QUrl::fromLocalFile(QString(HEADSET_DESK_QML_DIR) + "/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QVERIFY(window); QTRY_VERIFY(window->isVisible());
        for (int i = 0; i < 5; ++i) {
            std::function<QObject*(QQuickItem*)> find = [&](QQuickItem* item) -> QObject* {
                if (item->objectName() == "eqBand" + QString::number(i)) return item;
                for (auto* child : item->childItems()) if (auto* match = find(child)) return match;
                return nullptr;
            };
            auto* slider = find(window->contentItem());
            QVERIFY(slider); QCOMPARE(slider->property("enabled").toBool(), false);
        }
        auto* bass = window->findChild<QObject*>("clearBass");
        QVERIFY(bass); QVERIFY(!bass->property("enabled").toBool());
        window->resize(360, 420); QTest::qWait(100);
        QVERIFY(window->close() == false);
        QTRY_VERIFY(!window->isVisible());
        QVERIFY(device.state().value("connected").toBool());
        window->show(); device.refresh(); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("connected").toBool());
        QCOMPARE(device.state().value("firmware").toString(), "1.1.4");
    }
    void secondInstanceForwardsOpen() {
        const auto key = "headset-desk-test-" + QString::number(QCoreApplication::applicationPid());
        SingleInstance first(key);
        QVERIFY2(first.primary(), qPrintable(first.error()));
        QSignalSpy opened(&first, &SingleInstance::openRequested);
        QProcess second;
        second.start(QCoreApplication::applicationFilePath(), {"--forward-instance", key});
        QTRY_VERIFY_WITH_TIMEOUT(second.state() == QProcess::NotRunning, 10000);
        QCOMPARE(second.exitCode(), 0);
        // The OPEN message arrives over a local pipe and may land just after the second process exits.
        QTRY_COMPARE_WITH_TIMEOUT(opened.count(), 1, 5000);
    }
    void experimentalControlsAndReadback() {
        DeviceController device("ch720n");
        QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("controlsEnabled").toBool());
        QVERIFY(!device.state().value("dseeKnown").toBool()); // extras are not even queried before the opt-in
        device.setNoise(0); QTest::qWait(50);
        QCOMPARE(device.state().value("noiseMode").toInt(), 1);
        device.enableControls(true); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("controlsEnabled").toBool());
        device.setNoise(2, 20); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("noiseMode").toInt(), 2);
        QCOMPARE(device.state().value("ambient").toInt(), 20);
        QVERIFY(device.state().value("dseeKnown").toBool());
        QVERIFY(!device.state().value("speakKnown").toBool()); // no Speak-to-Chat on the CH720N
        device.setSpeakToChat(true); QTest::qWait(50);
        device.setDsee(true); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("dsee").toBool());
        device.setVoicePassthrough(true); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("voice").toBool());
        QCOMPARE(device.state().value("ambient").toInt(), 20);
        device.applyEqualizer(3, {-10,-5,0,5,10}); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("bass").toInt(), 3);
        QCOMPARE(device.state().value("bands").toList(), QVariantList({-10,-5,0,5,10}));
        device.applyEqualizer(0, {0,0,0,0,1.5}); QTest::qWait(50);
        QCOMPARE(device.state().value("bass").toInt(), 3);
        device.select(0); QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("controlsEnabled").toBool());
        device.toggleConnection(); QTRY_VERIFY(!device.busy());
        QVERIFY(!device.state().value("controlsEnabled").toBool());
    }
    void qmlControlActions() {
        const auto previousScheme = QGuiApplication::styleHints()->colorScheme();
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        DeviceController device("xm5"); QTRY_VERIFY(!device.busy());
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("device", &device);
        engine.rootContext()->setContextProperty("darkTheme", true);
        engine.rootContext()->setContextProperty("trayAvailable", true);
        engine.rootContext()->setContextProperty("reduceMotion", true);
        engine.load(QUrl::fromLocalFile(QString(HEADSET_DESK_QML_DIR) + "/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        std::function<QQuickItem*(QQuickItem*,QString)> find = [&](QQuickItem* item, QString name) -> QQuickItem* {
            if (item->objectName() == name) return item;
            for (auto* child : item->childItems()) if (auto* match = find(child, name)) return match;
            return nullptr;
        };
        auto* toggle = find(window->contentItem(), "controlsSwitch"); QVERIFY(toggle);
        toggle->setProperty("checked", true); QVERIFY(QMetaObject::invokeMethod(toggle, "toggled"));
        QTRY_VERIFY(!device.busy()); QVERIFY(device.state().value("controlsEnabled").toBool());
        auto* off = find(window->contentItem(), "noiseMode0"); QVERIFY(off); QVERIFY(off->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(off, "clicked")); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("noiseMode").toInt(), 0);
        auto* slider = find(window->contentItem(), "eqBand0"); QVERIFY(slider); QVERIFY(slider->isEnabled());
        slider->setProperty("value", 6); QVERIFY(QMetaObject::invokeMethod(slider, "moved"));
        auto* apply = find(window->contentItem(), "applyEq"); QVERIFY(apply); QTRY_VERIFY(apply->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(apply, "clicked")); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("bands").toList().at(0).toInt(), 6);
        device.setNoise(2, 10); QTRY_VERIFY(!device.busy());
        QCOMPARE(device.state().value("ambient").toInt(), 10);
        QVERIFY(device.state().value("speakKnown").toBool());
        device.setSpeakToChat(true); QTRY_VERIFY(!device.busy());
        QVERIFY(device.state().value("speak").toBool());
        auto* speak = find(window->contentItem(), "speakSwitch"); QVERIFY(speak); QVERIFY(speak->isEnabled());
        auto* nav = find(window->contentItem(), "navSound"); QVERIFY(nav);
        QVERIFY(QMetaObject::invokeMethod(nav, "clicked")); QTRY_COMPARE(window->property("page").toInt(), 1);
        const auto captures = qEnvironmentVariable("HEADSET_DESK_CAPTURE_DIR");
        if (!captures.isEmpty()) {
            window->resize(420, 820); QTest::qWait(300);
            QVERIFY(window->grabWindow().save(QDir(captures).filePath("experimental-dark-100.png")));
        }
        device.enableControls(false); QTRY_VERIFY(!device.busy());
        QVERIFY(!slider->isEnabled());
        QGuiApplication::styleHints()->setColorScheme(previousScheme);
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Headset Desk tests"); app.setOrganizationName("HeadsetDeskTests");
    app.setQuitOnLastWindowClosed(false);
    if (app.arguments().size() == 3 && app.arguments().at(1) == "--forward-instance") {
        SingleInstance instance(app.arguments().at(2));
        return instance.forwarded() ? 0 : 1;
    }
    QQuickStyle::setStyle("FluentWinUI3"); QQuickStyle::setFallbackStyle("Fusion");
    DesktopTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "DesktopTests.moc"
