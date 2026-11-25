#pragma once

#include "catalog.h"
#include "keygrab.h"
#include "store.h"

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

// The curriculum and the round in progress. QML reads it and draws it.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int generation READ generation NOTIFY updated)
    Q_PROPERTY(QString page READ page NOTIFY updated)
    Q_PROPERTY(bool helpOpen READ helpOpen NOTIFY updated)
    Q_PROPERTY(bool typing READ typing WRITE setTyping NOTIFY updated)
    Q_PROPERTY(QString error READ error NOTIFY updated)
    Q_PROPERTY(QString status READ status NOTIFY updated)
    Q_PROPERTY(QString confirm READ confirm NOTIFY updated)

    Q_PROPERTY(QString rankName READ rankName NOTIFY updated)
    Q_PROPERTY(QString rankTagline READ rankTagline NOTIFY updated)
    Q_PROPERTY(bool rankTop READ rankTop NOTIFY updated)
    Q_PROPERTY(int xp READ xp NOTIFY updated)
    Q_PROPERTY(int xpNext READ xpNext NOTIFY updated)
    Q_PROPERTY(double rankFraction READ rankFraction NOTIFY updated)
    Q_PROPERTY(int streakCurrent READ streakCurrent NOTIFY updated)
    Q_PROPERTY(int streakBest READ streakBest NOTIFY updated)
    Q_PROPERTY(QString streakLine READ streakLine NOTIFY updated)
    Q_PROPERTY(int mastered READ mastered NOTIFY updated)

    Q_PROPERTY(int rowCount READ rowCount NOTIFY updated)
    Q_PROPERTY(QString heroKicker READ heroKicker NOTIFY updated)
    Q_PROPERTY(QString heroTitle READ heroTitle NOTIFY updated)
    Q_PROPERTY(QString heroDetail READ heroDetail NOTIFY updated)
    Q_PROPERTY(QString heroAction READ heroAction NOTIFY updated)

    Q_PROPERTY(QString sessionKicker READ sessionKicker NOTIFY updated)
    Q_PROPERTY(QString sessionTitle READ sessionTitle NOTIFY updated)
    Q_PROPERTY(int sessionIndex READ sessionIndex NOTIFY updated)
    Q_PROPERTY(int sessionTotal READ sessionTotal NOTIFY updated)
    Q_PROPERTY(double sessionProgress READ sessionProgress NOTIFY updated)
    Q_PROPERTY(QString sessionAction READ sessionAction NOTIFY updated)
    Q_PROPERTY(QString sessionWhen READ sessionWhen NOTIFY updated)
    Q_PROPERTY(bool showKeys READ showKeys NOTIFY updated)
    Q_PROPERTY(QStringList keyParts READ keyParts NOTIFY updated)
    Q_PROPERTY(QStringList altKeyParts READ altKeyParts NOTIFY updated)
    Q_PROPERTY(QString keyLine READ keyLine NOTIFY updated)
    Q_PROPERTY(QString coach READ coach NOTIFY updated)
    Q_PROPERTY(QString coachTone READ coachTone NOTIFY updated)
    Q_PROPERTY(int combo READ combo NOTIFY updated)
    Q_PROPERTY(int lives READ lives NOTIFY updated)
    Q_PROPERTY(QString sessionHint READ sessionHint NOTIFY updated)

    Q_PROPERTY(QString resultKicker READ resultKicker NOTIFY updated)
    Q_PROPERTY(QString resultDetail READ resultDetail NOTIFY updated)
    Q_PROPERTY(QString resultAction READ resultAction NOTIFY updated)
    Q_PROPERTY(int resultHits READ resultHits NOTIFY updated)
    Q_PROPERTY(int resultTotal READ resultTotal NOTIFY updated)
    Q_PROPERTY(int resultAvgMs READ resultAvgMs NOTIFY updated)
    Q_PROPERTY(int resultXp READ resultXp NOTIFY updated)
    Q_PROPERTY(int resultCombo READ resultCombo NOTIFY updated)
    Q_PROPERTY(bool resultRankUp READ resultRankUp NOTIFY updated)
    Q_PROPERTY(QString resultRank READ resultRank NOTIFY updated)

    Q_PROPERTY(int atlasCount READ atlasCount NOTIFY updated)
    Q_PROPERTY(int atlasSelected READ atlasSelected NOTIFY updated)
    Q_PROPERTY(QString atlasQuery READ atlasQuery NOTIFY updated)
    Q_PROPERTY(QString atlasFilter READ atlasFilter NOTIFY updated)
    Q_PROPERTY(QStringList atlasFilters READ atlasFilters NOTIFY updated)

public:
    explicit Backend(const QString &statePath = {}, const QString &bindings = {}, bool capture = false,
                     const QString &hyprctl = {}, QObject *parent = nullptr);

    int generation() const { return m_generation; }
    QString page() const { return m_page; }
    bool helpOpen() const { return m_help; }
    bool typing() const { return m_typing; }
    void setTyping(bool typing);
    QString error() const { return m_error; }
    QString status() const { return m_status; }
    QString confirm() const { return m_confirm; }

    QString rankName() const;
    QString rankTagline() const;
    bool rankTop() const;
    int xp() const { return m_store.xp(); }
    int xpNext() const;
    double rankFraction() const;
    int streakCurrent() const { return m_store.streakCurrent(); }
    int streakBest() const { return m_store.streakBest(); }
    QString streakLine() const;
    int mastered() const { return m_store.mastered(); }

    int rowCount() const { return m_rows.size(); }
    QString heroKicker() const;
    QString heroTitle() const;
    QString heroDetail() const;
    QString heroAction() const;

    QString sessionKicker() const;
    QString sessionTitle() const;
    int sessionIndex() const;
    int sessionTotal() const;
    double sessionProgress() const;
    QString sessionAction() const;
    QString sessionWhen() const;
    bool showKeys() const;
    QStringList keyParts() const;
    QStringList altKeyParts() const;
    QString keyLine() const;
    QString coach() const;
    QString coachTone() const;
    int combo() const;
    int lives() const;
    QString sessionHint() const;

    QString resultKicker() const;
    QString resultDetail() const;
    QString resultAction() const;
    int resultHits() const { return m_result.hits; }
    int resultTotal() const { return m_result.total; }
    int resultAvgMs() const { return m_result.avgMs; }
    int resultXp() const { return m_result.xp; }
    int resultCombo() const { return m_result.bestCombo; }
    bool resultRankUp() const { return m_result.rankUp; }
    QString resultRank() const;

    int atlasCount() const { return m_atlas.size(); }
    int atlasSelected() const { return m_atlasSelected; }
    QString atlasQuery() const { return m_query; }
    QString atlasFilter() const { return m_filter; }
    QStringList atlasFilters() const;

    Q_INVOKABLE QVariantMap rowAt(int index) const;
    Q_INVOKABLE QVariantMap atlasAt(int index) const;
    Q_INVOKABLE void primaryAction();
    Q_INVOKABLE void move(int delta);
    Q_INVOKABLE void back();
    Q_INVOKABLE void askQuit();
    Q_INVOKABLE void toggleHelp();
    Q_INVOKABLE void showAtlas();
    Q_INVOKABLE void showHome();
    Q_INVOKABLE void setAtlasQuery(const QString &query);
    Q_INVOKABLE void setFilter(const QString &id);
    Q_INVOKABLE void cycleFilter(int delta);
    Q_INVOKABLE void hint();
    Q_INVOKABLE void skip();
    Q_INVOKABLE void choose(int index);
    Q_INVOKABLE void chooseAtlas(int index);
    Q_INVOKABLE void flush();
    Q_INVOKABLE void submitCombo(const QString &id);
    Q_INVOKABLE void submitKey(int key, int modifiers);
    Q_INVOKABLE QString solutionId() const;
    Q_INVOKABLE QString cardId() const;
    Q_INVOKABLE int itemWrong(const QString &id) const;
    Q_INVOKABLE QStringList sessionBelts() const;
    Q_INVOKABLE void retranslate();
    Q_INVOKABLE QString filterLabel(const QString &id) const;

    bool handleKey(const QKeyEvent *event, bool press);
    QByteArray reportJson() const;
    bool catalogOk() const;

signals:
    void updated();
    void quitRequested();

private:
    struct Row {
        QString kind;
        QString id;
        QString state;
    };
    struct Round {
        bool active = false;
        QString mode;
        QString belt;
        QString title;
        QList<Bind> cards;
        int index = 0;
        int hits = 0;
        int misses = 0;
        int triesLeft = 2;
        int lives = -1;
        int combo = 0;
        int bestCombo = 0;
        int xp = 0;
        bool showKeys = false;
        QString coach;
        QString tone = "idle";
        QString after = "advance";
        QList<int> hitMs;
        int rankIndex = 0;
    };
    struct Result {
        QString mode;
        QString belt;
        QString title;
        int hits = 0;
        int total = 0;
        int misses = 0;
        int avgMs = 0;
        int xp = 0;
        int bestCombo = 0;
        bool cleared = false;
        bool completed = false;
        bool rankUp = false;
        int rankIndex = 0;
        QString next = "home";
        QString nextBelt;
    };

    void touch();
    void loadLiveBindings();
    void rebuildRows();
    void rebuildAtlas();
    void selectInitial();
    QString beltTitle(const QString &id) const;
    QString beltBlurb(const QString &id) const;
    QVariantMap viewFor(const Row &row) const;
    bool startRow(const Row &row);
    bool begin(const QString &mode, const QString &belt, const QString &title, QList<Bind> cards, int tries, int lives);
    void startCard();
    void judge(const Combo &got);
    void onFlash();
    void advance();
    void finish(bool completed);
    void leaveRound();
    const Bind *current() const;
    QList<Bind> pool() const;
    QList<Bind> bindsForIds(const QStringList &ids) const;
    QString grabError() const;
    static QString hitLine();
    static QString missLine();

    Store m_store;
    Keygrab m_grab;
    QList<Bind> m_binds;
    QList<Row> m_rows;
    QList<int> m_atlas;
    QString m_page = "home";
    QString m_error;
    QString m_status;
    QString m_confirm;
    QString m_query;
    QString m_filter = "all";
    bool m_help = false;
    bool m_typing = false;
    bool m_capture = false;
    int m_selected = 0;
    int m_atlasSelected = 0;
    int m_generation = 0;
    Round m_round;
    Result m_result;
    QElapsedTimer m_clock;
    QTimer m_flash;
};
