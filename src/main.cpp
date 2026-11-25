// Omarchy Dojo — train the desktop's shortcuts until the hands know them.

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QKeyEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>

#include "backend.h"
#include "catalog.h"
#include "theme.h"
#include "translator.h"

namespace {

class DojoApp : public QGuiApplication {
public:
    using QGuiApplication::QGuiApplication;
    Backend *backend = nullptr;

protected:
    bool notify(QObject *receiver, QEvent *event) override
    {
        if (backend && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {
            if (backend->handleKey(static_cast<QKeyEvent *>(event), event->type() == QEvent::KeyPress))
                return true;
        }
        return QGuiApplication::notify(receiver, event);
    }
};

struct Options {
    bool check = false;
    QString preview;
    QString lang;
};

Options parse(int argc, char **argv)
{
    Options options;
    const QStringList args = [&] {
        QStringList out;
        for (int i = 1; i < argc; ++i)
            out.append(QString::fromLocal8Bit(argv[i]));
        return out;
    }();
    options.check = args.contains("--check");
    const int preview = args.indexOf("--preview");
    if (preview >= 0 && preview + 1 < args.size())
        options.preview = args.at(preview + 1);
    const int lang = args.indexOf("--lang");
    if (lang >= 0 && lang + 1 < args.size())
        options.lang = args.at(lang + 1);
    return options;
}

} // namespace

int main(int argc, char *argv[])
{
    const Options options = parse(argc, argv);
    if (options.check || !options.preview.isEmpty())
        qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_FORCE_STDERR_LOGGING", "1");

    if (!options.check && options.preview.isEmpty()) {
        QLocalSocket existing;
        existing.connectToServer(QStringLiteral("omarchy-dojo"));
        if (existing.waitForConnected(200)) {
            existing.write("raise");
            existing.flush();
            existing.waitForBytesWritten(200);
            return 0;
        }
    }

    DojoApp app(argc, argv);
    app.setApplicationName("omarchy-dojo");
    app.setApplicationVersion("1.0.0");
    // The Wayland app id matches omarchy-dojo.desktop, so the launcher icon
    // and the Hyprland window rule find this window.
    app.setDesktopFileName("omarchy-dojo");
    app.setWindowIcon(QIcon::fromTheme("omarchy-dojo"));
    QQuickStyle::setStyle("Material");

    if (options.check) {
        Backend backend({}, {}, false);
        fprintf(stdout, "%s\n", backend.reportJson().constData());
        return backend.catalogOk() ? 0 : 1;
    }

    QTemporaryDir scratch;
    QString languageFile;
    QString themeDirectory;
    QString statePath;
    QString bindings;
    if (!options.preview.isEmpty()) {
        languageFile = scratch.filePath("language");
        QFile language(languageFile);
        if (language.open(QIODevice::WriteOnly))
            language.write((options.lang.isEmpty() ? QStringLiteral("en") : options.lang).toUtf8() + '\n');
        themeDirectory = scratch.filePath("theme-root");
        QDir().mkpath(themeDirectory);
        statePath = scratch.filePath("state.json");
        bindings = previewBindings();
    }

    Translator translator(languageFile);
    app.installTranslator(&translator);
    Theme theme(themeDirectory);
    // The preview is offscreen and must not steal the live Hyprland submap.
    Backend backend(statePath, bindings, options.preview.isEmpty());

    if (!options.preview.isEmpty())
        backend.primaryAction();

    QQmlApplicationEngine engine;
    QObject::connect(&translator, &Translator::changed, &engine, &QQmlEngine::retranslate);
    QObject::connect(&translator, &Translator::changed, &backend, &Backend::retranslate);
    QObject::connect(&backend, &Backend::quitRequested, &app, &QGuiApplication::quit);
    engine.rootContext()->setContextProperty("translator", &translator);
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);
    app.backend = &backend;

    QLocalServer server;
    if (options.preview.isEmpty()) {
        QLocalServer::removeServer(QStringLiteral("omarchy-dojo"));
        server.listen(QStringLiteral("omarchy-dojo"));
        QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
            if (auto *socket = server.nextPendingConnection())
                socket->deleteLater();
            const auto roots = engine.rootObjects();
            if (roots.isEmpty())
                return;
            if (auto *window = qobject_cast<QWindow *>(roots.first())) {
                window->show();
                window->raise();
                window->requestActivate();
            }
        });
    }

    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    if (!options.preview.isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (!window)
            return 1;
        // The first swapped frame can still be empty. Grab once layout has run.
        QTimer::singleShot(200, &app, [window, &app, options] {
            const QImage image = window->grabWindow();
            if (image.isNull() || !image.save(options.preview))
                fprintf(stderr, "could not write %s\n", qPrintable(options.preview));
            app.quit();
        });
        QTimer::singleShot(3000, &app, &QGuiApplication::quit);
    }

    return app.exec();
}
