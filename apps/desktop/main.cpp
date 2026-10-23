#include "DeviceController.h"
#include "SingleInstance.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleHints>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QActionGroup>
#include <QPainter>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <array>

namespace {
struct InstallLock {
    HANDLE handle{};
    ~InstallLock() { if (handle) CloseHandle(handle); }
};
QIcon headsetIcon() {
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        QPixmap pixmap(size, size); pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap); painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(size / 32.0, size / 32.0);
        painter.setPen(QPen(QColor("#3289cf"), 3, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(QRectF(6, 4, 20, 22), 0, 180 * 16);
        painter.setBrush(QColor("#3289cf")); painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(4, 15, 6, 12), 2, 2);
        painter.drawRoundedRect(QRectF(22, 15, 6, 12), 2, 2);
        icon.addPixmap(pixmap);
    }
    return icon;
}

// Match the title bar to the page (Windows 11 title-bar colour attributes; ignored on older builds)
// and follow the light/dark setting, so the window reads as one surface.
void applyWindowChrome(QWindow* window, bool dark) {
    using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    static const auto setAttribute = [] {
        const HMODULE module = LoadLibraryW(L"dwmapi.dll");
        return module ? reinterpret_cast<DwmSetWindowAttributeFn>(reinterpret_cast<void*>(GetProcAddress(module, "DwmSetWindowAttribute"))) : nullptr;
    }();
    if (!setAttribute || !window) return;
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL immersiveDark = dark ? TRUE : FALSE;
    const COLORREF caption = dark ? RGB(0x20, 0x20, 0x20) : RGB(0xf3, 0xf3, 0xf3);
    const COLORREF text = dark ? RGB(0xff, 0xff, 0xff) : RGB(0x1a, 0x1a, 0x1a);
    setAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &immersiveDark, sizeof(immersiveDark));
    setAttribute(hwnd, 35 /* DWMWA_CAPTION_COLOR */, &caption, sizeof(caption));
    setAttribute(hwnd, 36 /* DWMWA_TEXT_COLOR */, &text, sizeof(text));
}
}

int main(int argc, char* argv[]) {
    QElapsedTimer startup; startup.start();
    QApplication app(argc, argv);
    app.setApplicationName("Headset Desk"); app.setOrganizationName("HeadsetDesk");
    app.setApplicationVersion("0.6.0-beta.1");
    app.setQuitOnLastWindowClosed(false);
    QCommandLineParser parser;
    parser.setApplicationDescription("Headset Desk — Windows headphone controls");
    parser.addHelpOption(); parser.addVersionOption();
    parser.addOption({"simulate", "Internal offline preview: xm5, ch720n or idle.", "model"});
    parser.addOption({"theme", "Internal preview theme: light or dark.", "theme"});
    parser.addOption({"capture", "Internal preview PNG output (requires simulation).", "path"});
    parser.addOption({"metrics", "Internal preview measurements JSON (requires simulation).", "path"});
    parser.addOption({"ready-file", "Internal first-frame marker (requires simulation).", "path"});
    parser.addOption({"page", "Internal preview page: 0 Headphones, 1 Sound, 2 Features, 3 Device.", "number"});
    parser.addOption({"headphone-button", "Internal preview: press the simulated headset's noise button (0 Off, 1 NC, 2 Ambient) before capturing.", "mode"});
    parser.addOption({"controls", "Internal preview: turn on Allow changes in the simulation before capturing."});
    parser.addOption({"exit-after-capture", "Exit the offline preview after recording."});
    parser.process(app);
    const auto simulation = parser.value("simulate");
    const auto theme = parser.value("theme");
    if ((!simulation.isEmpty() && simulation != "xm5" && simulation != "ch720n" && simulation != "idle") ||
        (!theme.isEmpty() && theme != "light" && theme != "dark") ||
        (simulation.isEmpty() && (parser.isSet("theme") || parser.isSet("capture") || parser.isSet("metrics") || parser.isSet("ready-file") || parser.isSet("exit-after-capture") || parser.isSet("page") || parser.isSet("controls") || parser.isSet("headphone-button"))) ||
        (parser.isSet("headphone-button") && !QStringList{"0", "1", "2"}.contains(parser.value("headphone-button"))) ||
        (parser.isSet("page") && !QStringList{"0", "1", "2", "3"}.contains(parser.value("page"))) ||
        (parser.isSet("exit-after-capture") && !parser.isSet("capture") && !parser.isSet("metrics"))) return 2;

    SingleInstance instance(simulation.isEmpty() ? "headset-desk-v1" : "headset-desk-preview-v1");
    if (!instance.primary()) {
        if (!instance.forwarded()) {
            std::cerr << instance.error().toStdString() << '\n';
            if (simulation.isEmpty()) QMessageBox::warning(nullptr, "Headset Desk", instance.error());
        }
        return instance.forwarded() ? 0 : 3;
    }
    InstallLock installLock;
    if (simulation.isEmpty()) installLock.handle = CreateMutexW(nullptr, FALSE, L"Local\\HeadsetDeskRunningV1");
    if (!theme.isEmpty()) app.styleHints()->setColorScheme(theme == "dark" ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    QQuickStyle::setStyle("FluentWinUI3");
    QQuickStyle::setFallbackStyle("Fusion");
    const auto icon = headsetIcon(); app.setWindowIcon(icon);
    DeviceController controller(simulation);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("device", &controller);
    const auto updateTheme = [&] {
        engine.rootContext()->setContextProperty("darkTheme", app.styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    };
    updateTheme();
    QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, &app, updateTheme);
    // Respect Windows "Animation effects": when off, the UI skips decorative motion.
    BOOL animations = TRUE;
    if (!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0)) animations = TRUE;
    engine.rootContext()->setContextProperty("reduceMotion", animations == FALSE);
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    engine.rootContext()->setContextProperty("trayAvailable", trayAvailable);
    engine.loadFromModule("HeadsetDesk", "Main");
    if (engine.rootObjects().isEmpty()) return 4;
    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!window) return 4;
    window->setIcon(icon);
    if (parser.isSet("page")) window->setProperty("page", parser.value("page").toInt());
    applyWindowChrome(window, app.styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, window, [window, &app] {
        applyWindowChrome(window, app.styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    });

    QSystemTrayIcon tray(icon);
    QMenu menu;
    auto* status = menu.addAction("Headset Desk · Ready to connect"); status->setEnabled(false);
    auto* open = menu.addAction("Open Headset Desk");
    // Quick noise switching from the tray, like a menu-bar app. Same rules as the window:
    // the items work only while Allow changes is on for the connection.
    menu.addSeparator();
    auto* noiseHint = menu.addAction("Noise control"); noiseHint->setEnabled(false); noiseHint->setVisible(false);
    QActionGroup noiseGroup(&menu); noiseGroup.setExclusive(true);
    const std::array<std::pair<int, const char*>, 3> noiseModes{{{1, "Noise Cancelling"}, {2, "Ambient"}, {0, "Off"}}};
    std::array<QAction*, 3> noiseActions{};
    for (std::size_t i = 0; i < noiseModes.size(); ++i) {
        auto* action = menu.addAction(noiseModes[i].second);
        action->setCheckable(true); action->setVisible(false); noiseGroup.addAction(action);
        const int mode = noiseModes[i].first;
        QObject::connect(action, &QAction::triggered, &controller, [&controller, mode] { controller.setNoise(mode); });
        noiseActions[i] = action;
    }
    menu.addSeparator(); auto* quit = menu.addAction("Quit");
    tray.setContextMenu(&menu); tray.setToolTip("Headset Desk");
    const auto showWindow = [&] {
        window->showNormal(); window->raise(); window->requestActivate(); controller.refresh();
    };
    QObject::connect(open, &QAction::triggered, &app, showWindow);
    QObject::connect(&instance, &SingleInstance::openRequested, &app, showWindow);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) showWindow();
    });
    QObject::connect(quit, &QAction::triggered, &controller, &DeviceController::quit);
    QObject::connect(&controller, &DeviceController::finished, &app, &QCoreApplication::quit);
    QObject::connect(&controller, &DeviceController::changed, &app, [&] {
        const auto state = controller.state();
        const auto text = state.value("model").toString() + " · " + state.value("status").toString();
        status->setText(text); tray.setToolTip("Headset Desk\n" + text);
        const bool connected = state.value("connected").toBool() && state.value("noiseKnown").toBool();
        const bool canChange = connected && state.value("controlsEnabled").toBool() && !controller.busy();
        noiseHint->setText(!connected ? "Noise control" : canChange ? "Noise control" : "Noise control (turn on Allow changes)");
        for (std::size_t i = 0; i < noiseActions.size(); ++i) {
            noiseActions[i]->setVisible(connected);
            noiseActions[i]->setEnabled(canChange);
            noiseActions[i]->setChecked(connected && state.value("noiseMode").toInt() == noiseModes[i].first);
        }
        noiseHint->setVisible(connected);
    });
    // While the window is visible, also poll the noise mode so headphone button presses show up
    // even if the headset doesn't announce them; hidden in the tray, rely on its own notifications.
    QObject::connect(window, &QWindow::visibleChanged, &controller, [&controller](bool visible) { controller.setWatching(visible); });
    controller.setWatching(window->isVisible());
    if (trayAvailable) tray.show();

    // Internal preview path exercises the real window and worker without Windows Bluetooth.
    // Own-window capture records rendered pixels at the requested process DPI.
    qint64 firstFrameMs = -1;
    QObject::connect(window, &QQuickWindow::frameSwapped, &app, [&] {
        if (firstFrameMs >= 0) return;
        firstFrameMs = startup.elapsed();
        if (parser.isSet("ready-file")) {
            QFile ready(parser.value("ready-file"));
            if (ready.open(QIODevice::WriteOnly)) ready.write(QJsonDocument(QJsonObject{{"firstFrameMs", firstFrameMs}}).toJson());
        }
    }, Qt::QueuedConnection);
    bool recorded = false;
    bool controlsRequested = false;
    qint64 buttonPressedAt = -1;
    QTimer previewTimer;
    if (parser.isSet("capture") || parser.isSet("metrics")) {
        previewTimer.setInterval(250);
        QObject::connect(&previewTimer, &QTimer::timeout, &app, [&] {
            if (recorded || controller.busy() || firstFrameMs < 0 || startup.elapsed() < 6000) return;
            if (parser.isSet("controls") && !controller.state().value("controlsEnabled").toBool()) {
                if (!controlsRequested) { controlsRequested = true; controller.enableControls(true); }
                return; // wait for the opt-in and the extra readings to settle
            }
            if (parser.isSet("headphone-button")) {
                if (buttonPressedAt < 0) { buttonPressedAt = startup.elapsed(); controller.simulateHeadphoneButton(parser.value("headphone-button").toInt()); }
                if (startup.elapsed() - buttonPressedAt < 2200) return; // past the app-change echo window, toast showing
            }
            recorded = true; previewTimer.stop();
            PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb = sizeof(memory);
            GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory));
            const auto image = window->grabWindow();
            bool success = !image.isNull();
            if (parser.isSet("capture")) success = success && image.save(parser.value("capture"));
            if (parser.isSet("metrics")) {
                QJsonObject metrics{{"version", app.applicationVersion()}, {"qt", qVersion()},
                    {"style", QQuickStyle::name()}, {"theme", theme}, {"simulation", simulation},
                    {"firstFrameMs", firstFrameMs}, {"sampleAtMs", startup.elapsed()},
                    {"workingSetBytes", static_cast<double>(memory.WorkingSetSize)},
                    {"privateBytes", static_cast<double>(memory.PrivateUsage)},
                    {"devicePixelRatio", window->devicePixelRatio()}, {"pixelWidth", image.width()}, {"pixelHeight", image.height()},
                    {"logicalWidth", window->width()}, {"logicalHeight", window->height()}, {"trayAvailable", trayAvailable},
                    {"captureSuccess", success}};
                QFile output(parser.value("metrics"));
                success = output.open(QIODevice::WriteOnly) && output.write(QJsonDocument(metrics).toJson()) > 0 && success;
            }
            if (!success) app.exit(5);
            else if (parser.isSet("exit-after-capture")) controller.quit();
        });
        previewTimer.start();
        QTimer::singleShot(30000, &app, [&] { if (!recorded) app.exit(6); });
    }
    return app.exec();
}
