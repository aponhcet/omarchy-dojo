#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "catalog.h"
#include "combo.h"
#include "store.h"
#include "theme.h"
#include "translator.h"

class DojoTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QQuickStyle::setStyle("Material");
    }

    void comboParsesChords()
    {
        QCOMPARE(parseCombo("SUPER + K").id(), QString("SUPER+K"));
        QCOMPARE(parseCombo("SUPER SHIFT + RETURN").id(), QString("SUPER+SHIFT+RETURN"));
        QCOMPARE(parseCombo("SUPER SHIFT + return").label(), QString("Super + Shift + Enter"));

        const auto alternatives = parseAlternatives("SUPER + W / SUPER + Q");
        QCOMPARE(alternatives.size(), 2);
        QCOMPARE(alternatives.at(0).id(), QString("SUPER+W"));
        QCOMPARE(alternatives.at(1).id(), QString("SUPER+Q"));

        const auto binds = loadBinds("SUPER + W / SUPER + Q                       → Close window\n");
        QCOMPARE(binds.size(), 1);
        QCOMPARE(binds.at(0).id, QString("SUPER+W|close window"));
        QVERIFY(binds.at(0).matches(parseCombo("SUPER+W")));
        QVERIFY(binds.at(0).matches(parseCombo("SUPER+Q")));
        QVERIFY(binds.at(0).matches(comboFromQt(Qt::Key_Q, Qt::MetaModifier)));
    }

    void catalogKeepsLessonOrderAndDigitBoundaries()
    {
        const auto binds = loadBinds(sampleCatalog());
        const auto essentials = beltBinds(binds, "essentials");
        QStringList lesson;
        for (const auto &bind : essentials)
            lesson.append(bind.description);
        QCOMPARE(lesson, QStringList({
            "Omarchy menu", "Terminal", "Browser", "Close window", "Keybindings", "File manager", "System menu",
        }));

        const auto desks = beltBinds(binds, "workspaces");
        QVERIFY(!desks.isEmpty());
        QCOMPARE(desks.first().description, QString("Switch to workspace 1"));
        for (const auto &bind : desks)
            QVERIFY2(!bind.description.contains("10"), qPrintable(bind.description));

        bool privateBrowser = false;
        bool dojo = false;
        bool mouse = false;
        for (const auto &bind : binds) {
            if (bind.description == "Browser (private)")
                privateBrowser = bind.belt == "essentials";
            if (bind.description.compare("Shortcut Dojo", Qt::CaseInsensitive) == 0)
                dojo = true;
            if (bind.description == "Poke") {
                mouse = true;
                QVERIFY(!bind.practiceable);
            }
        }
        QVERIFY(!privateBrowser);
        QVERIFY(!dojo);
        QVERIFY(mouse);
    }

    void oldProgressBecomesBeltsAndXp()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("state.json");
        QVERIFY(writeFile(path, R"({
            "version": 1,
            "streak": {"current": 1, "best": 1, "last_active": "2026-08-28", "days": ["2026-08-28"]},
            "training": {"completed": ["essentials"]},
            "items": {
                "SUPER+W|close window": {"correct": 3, "seen": 4, "wrong": 1, "ease": 2.2, "interval": 2},
                "SUPER+K|keybindings": {"correct": 7, "seen": 7, "wrong": 0}
            },
            "sessions": []
        })"));

        Store store(path);
        QCOMPARE(store.warning(), QString());
        QVERIFY(store.taught("essentials"));
        QVERIFY(store.cleared("essentials"));
        QCOMPARE(store.correctCount("SUPER+W|close window"), 3);
        QCOMPARE(store.xp(), 50 + (3 + 7) * 5);
        QCOMPARE(store.streakCurrent(), 0);
        QCOMPARE(store.streakBest(), 1);
        QCOMPARE(QString::fromUtf8(kRanks[store.rank().index].name), QString("Super Curious"));

        Store again(path);
        QCOMPARE(again.xp(), store.xp());
    }

    void teachMissDoesNotScoreAndEscapeAsks()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), previewBindings(), false);
        backend.primaryAction();
        QCOMPARE(backend.page(), QString("session"));
        QCOMPARE(backend.sessionKicker(), QString("Teach"));
        QCOMPARE(backend.sessionAction(), QString("Omarchy menu"));
        QVERIFY(backend.showKeys());

        backend.submitCombo("CTRL+Z");
        QCOMPARE(backend.coachTone(), QString("miss"));
        QCOMPARE(backend.itemWrong(backend.cardId()), 0);
        QCOMPARE(backend.xp(), 0);
        backend.flush();
        QCOMPARE(backend.sessionAction(), QString("Omarchy menu"));

        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QVERIFY(backend.handleKey(&escape, true));
        QCOMPARE(backend.confirm(), QString("leave"));
        QCOMPARE(backend.page(), QString("session"));

        QKeyEvent stay(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QVERIFY(backend.handleKey(&stay, true));
        QCOMPARE(backend.confirm(), QString());
        QCOMPARE(backend.page(), QString("session"));
    }

    void superEscapeOnTheSystemMenuIsAHit()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), previewBindings(), false);
        backend.primaryAction();
        while (backend.page() == "session" && backend.sessionAction() != "System menu") {
            backend.submitCombo(backend.solutionId());
            backend.flush();
        }
        QCOMPARE(backend.sessionAction(), QString("System menu"));
        backend.submitKey(Qt::Key_Escape, Qt::MetaModifier);
        QCOMPARE(backend.confirm(), QString());
        QCOMPARE(backend.coachTone(), QString("hit"));
        QCOMPARE(backend.itemWrong(backend.cardId()), 0);
    }

    void teachThenPracticeUnlocksTheNextBelt()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), curriculum(), false);
        backend.primaryAction();
        answer(backend, true);
        QCOMPARE(backend.page(), QString("result"));
        QCOMPARE(backend.resultKicker(), QString("Lesson complete"));
        QVERIFY(backend.resultRankUp());
        QVERIFY(backend.resultDetail().contains("Super Curious"));
        QVERIFY(backend.resultDetail().contains("Next they hide"));
        QCOMPARE(backend.resultAction(), QString("Practice"));

        backend.primaryAction();
        QCOMPARE(backend.sessionKicker(), QString("Practice"));
        QVERIFY(!backend.showKeys());
        QCOMPARE(backend.keyParts(), QStringList({QStringLiteral("?")}));
        backend.hint();
        QVERIFY(backend.showKeys());

        answer(backend, true);
        QCOMPARE(backend.resultKicker(), QString("Belt cleared"));
        QVERIFY(backend.resultDetail().contains("Windows is open"));
        QCOMPARE(backend.resultAction(), QString("Teach Windows"));

        backend.showHome();
        QCOMPARE(backend.rowAt(0).value("state").toString(), QString("cleared"));
        QCOMPARE(backend.rowAt(1).value("id").toString(), QString("windows"));
        QCOMPARE(backend.rowAt(1).value("state").toString(), QString("teach"));
        bool sparring = false;
        for (int i = 0; i < backend.rowCount(); ++i)
            sparring = sparring || backend.rowAt(i).value("kind").toString() == "spar";
        QVERIFY(sparring);
    }

    void failedPracticeKeepsTheBeltAndSparringWaits()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), curriculum(), false);
        QVERIFY(!hasSparring(backend));
        backend.primaryAction();
        answer(backend, true);
        backend.primaryAction();
        answer(backend, false);
        QCOMPARE(backend.page(), QString("result"));
        QCOMPARE(backend.resultKicker(), QString("Not yet"));
        QCOMPARE(backend.resultHits(), 0);
        QCOMPARE(backend.resultAction(), QString("Try again"));

        backend.showHome();
        QCOMPARE(backend.rowAt(0).value("state").toString(), QString("practice"));
        QCOMPARE(backend.rowAt(1).value("state").toString(), QString("locked"));
        QVERIFY(!hasSparring(backend));
    }

    void sparringUsesOnlyClearedBelts()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), curriculum(), false);
        backend.primaryAction();
        answer(backend, true);
        backend.primaryAction();
        answer(backend, true);
        backend.showHome();

        int daily = -1;
        for (int i = 0; i < backend.rowCount(); ++i)
            if (backend.rowAt(i).value("id").toString() == "daily")
                daily = i;
        QVERIFY(daily >= 0);
        backend.choose(daily);
        QCOMPARE(backend.page(), QString("session"));
        QCOMPARE(backend.sessionKicker(), QString("Daily"));
        const auto belts = backend.sessionBelts();
        QVERIFY(!belts.isEmpty());
        for (const auto &belt : belts)
            QCOMPARE(belt, QString("essentials"));
    }

    void bossSpendsThreeLives()
    {
        QTemporaryDir dir;
        Backend backend(dir.filePath("state.json"), curriculum(), false);
        backend.primaryAction();
        answer(backend, true);
        backend.primaryAction();
        answer(backend, true);
        backend.showHome();
        int boss = -1;
        for (int i = 0; i < backend.rowCount(); ++i)
            if (backend.rowAt(i).value("id").toString() == "boss")
                boss = i;
        QVERIFY(boss >= 0);
        backend.choose(boss);
        QCOMPARE(backend.lives(), 3);
        backend.submitCombo("CTRL+Z");
        QCOMPARE(backend.lives(), 2);
        backend.flush();
        backend.submitCombo("CTRL+Z");
        QCOMPARE(backend.lives(), 1);
        backend.flush();
        backend.submitCombo("CTRL+Z");
        QCOMPARE(backend.lives(), 0);
        backend.flush();
        QCOMPARE(backend.page(), QString("result"));
        QCOMPARE(backend.resultKicker(), QString("Session"));
    }

    void failedSubmapStaysHomeAndAGoodOneResets()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto broken = dir.filePath("broken");
        const auto healthy = dir.filePath("hyprctl");
        const auto log = dir.filePath("log");
        QVERIFY(writeFile(broken, "#!/bin/sh\necho 'no submap' >&2\nexit 1\n"));
        QVERIFY(writeFile(healthy, "#!/bin/sh\nprintf '%s\\n' \"$@\" >> '" + log.toUtf8() + "'\nexit 0\n"));
        QVERIFY(QFile::setPermissions(broken, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(QFile::setPermissions(healthy, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        Backend blocked(dir.filePath("blocked.json"), previewBindings(), true, broken);
        blocked.primaryAction();
        QCOMPARE(blocked.page(), QString("home"));
        QVERIFY(blocked.status().contains("submap"));

        Backend live(dir.filePath("live.json"), previewBindings(), true, healthy);
        live.primaryAction();
        QCOMPARE(live.page(), QString("session"));
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QVERIFY(live.handleKey(&escape, true));
        QKeyEvent confirm(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QVERIFY(live.handleKey(&confirm, true));
        QCOMPARE(live.page(), QString("home"));

        QFile file(log);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto text = QString::fromUtf8(file.readAll());
        QVERIFY(text.contains("hl.dsp.submap(\"omarchy-dojo\")"));
        QVERIFY(text.contains("hl.dsp.submap(\"reset\")"));
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QCOMPARE(theme.radius(), 0);
    }

    void themeReadsAccent()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QVERIFY(writeFile(dir.path() + "/theme/colors.toml", "accent = \"#112233\"\n"));
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json");
        const QString command = dir.filePath("hyprctl");
        QVERIFY(writeFile(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(writeFile(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(writeFile(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.filePath("language"), dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        QVERIFY(writeFile(dir.filePath("language"), "pt-BR\n"));
        QVERIFY(writeFile(dir.filePath("pt.json"), R"({"Save": "Guardar", "Open": "Abrir"})"));
        QVERIFY(writeFile(dir.filePath("pt-BR.json"), R"({"Save": "Salvar"})"));

        Translator translator(dir.filePath("language"), dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        QVERIFY(writeFile(dir.filePath("language"), "ru\n"));
        QVERIFY(writeFile(dir.filePath("en.json"), R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})"));
        QVERIFY(writeFile(dir.filePath("ru.json"),
                          R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})"));

        Translator translator(dir.filePath("language"), dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
        QCOMPARE(Translator::pluralCategory("da", 1), QString("one"));
        QCOMPARE(Translator::pluralCategory("da", 2), QString("other"));
    }

    void windowShowsTheLessonThenTheNextBind()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        Translator translator(dir.filePath("language"), QStringLiteral(":/i18n"));
        Backend backend(dir.filePath("state.json"), previewBindings(), false);

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("translator", &translator);
        engine.rootContext()->setContextProperty("theme", &theme);
        engine.rootContext()->setContextProperty("backend", &backend);
        QObject::connect(&engine, &QQmlEngine::warnings, &engine, [](const QList<QQmlError> &warnings) {
            for (const auto &warning : warnings)
                qWarning().noquote() << warning.toString();
        });
        engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
        QVERIFY(!engine.rootObjects().isEmpty());

        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        backend.primaryAction();
        QObject *label = nullptr;
        QTRY_VERIFY((label = window->findChild<QObject *>("actionLabel")));
        QCOMPARE(label->property("text").toString(), QString("Omarchy menu"));

        auto *keyRow = window->findChild<QQuickItem *>("keyRow");
        QVERIFY(keyRow);
        QStringList caps;
        collectText(keyRow, "keycap", &caps);
        QVERIFY2(caps.contains("Super") && caps.contains("Space"), qPrintable(debugTree(keyRow)));

        backend.submitCombo("SUPER+SPACE");
        QCOMPARE(backend.coachTone(), QString("hit"));
        backend.flush();
        QCOMPARE(backend.sessionAction(), QString("Terminal"));
        QTRY_COMPARE(label->property("text").toString(), QString("Terminal"));
    }

private:
    static bool writeFile(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static QString sampleCatalog()
    {
        return QString::fromUtf8(
            "SUPER + SPACE                       → Omarchy menu\n"
            "SUPER + RETURN                      → Terminal\n"
            "SUPER SHIFT + RETURN                → Browser\n"
            "SUPER + SHIFT + ALT + B             → Browser (private)\n"
            "SUPER + W / SUPER + Q               → Close window\n"
            "SUPER + K                           → Keybindings\n"
            "SUPER SHIFT + F                     → File manager\n"
            "SUPER + ESCAPE                      → System menu\n"
            "SUPER + K                           → Shortcut Dojo\n"
            "SUPER + mouse:272                   → Poke\n"
            "SUPER + 1                           → Switch to workspace 1\n"
            "SUPER + 0                           → Switch to workspace 10\n"
            "SUPER + 2                           → Switch to workspace 2\n"
            "SUPER + 3                           → Switch to workspace 3\n"
            "SUPER + TAB                         → Next workspace\n"
            "SUPER + SHIFT + TAB                 → Previous workspace\n"
            "SUPER + O                           → Former workspace\n"
            "SUPER + SHIFT + 1                   → Move window to workspace 1\n"
            "SUPER + SHIFT + 2                   → Move window to workspace 2\n"
            "SUPER + SHIFT + 0                   → Move window to workspace 10\n");
    }

    static QString curriculum()
    {
        return previewBindings() + QStringLiteral("SUPER + F                           → Full screen\n");
    }

    static void answer(Backend &backend, bool correct)
    {
        int guard = 0;
        while (backend.page() == "session" && guard < 40) {
            if (correct)
                backend.submitCombo(backend.solutionId());
            else
                backend.submitCombo("CTRL+Z");
            if (backend.page() == "session")
                backend.flush();
            ++guard;
        }
    }

    static void collectText(QQuickItem *item, const QString &name, QStringList *out)
    {
        if (item->objectName() == name)
            out->append(item->property("text").toString());
        for (auto *child : item->childItems())
            collectText(child, name, out);
    }

    static QString debugTree(QQuickItem *item, int depth = 0)
    {
        QString dump = QString(depth * 2, ' ') + item->metaObject()->className() + " " + item->objectName()
            + " kids " + QString::number(item->childItems().size()) + "\n";
        for (auto *child : item->childItems())
            dump += debugTree(child, depth + 1);
        return dump;
    }

    static bool hasSparring(const Backend &backend)
    {
        for (int i = 0; i < backend.rowCount(); ++i)
            if (backend.rowAt(i).value("kind").toString() == "spar")
                return true;
        return false;
    }
};

QTEST_MAIN(DojoTests)
#include "omarchy-dojo_tests.moc"
