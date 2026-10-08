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
        QCOMPARE(opened.count(), 1);
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
