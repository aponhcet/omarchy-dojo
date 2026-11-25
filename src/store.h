#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct RankDef {
    int xp;
    const char *name;
    const char *tagline;
};

// XP thresholds. A finished belt has to feel like a step, so the early ranks
// sit inside a single evening of practice.
inline const RankDef kRanks[] = {
    {0, "Mouse Refugee", "Still reaching."},
    {40, "Super Curious", "Found the key."},
    {120, "Tiler", "Windows obey."},
    {280, "Workspace Nomad", "Ten desks, no map."},
    {500, "Hypr Mage", "Binds before thought."},
    {800, "Omarchist", "The desktop is an instrument."},
    {1200, "Sensei", "Teach the next one."},
};

struct RankPlace {
    int index = 0;
    int floor = 0;
    int next = 40;
    bool top = false;
};

struct ReviewResult {
    bool correct = false;
    bool newBest = false;
    int bestMs = 0;
};

// Progress lives in the same file the earlier Dojo wrote, so a rewrite does
// not throw away streaks, due dates, or the belts already finished.
class Store {
public:
    explicit Store(QString path = {});

    QString path() const { return m_path; }
    QString warning() const { return m_warning; }
    int xp() const;
    void addXp(int amount);
    int mastered() const;

    bool taught(const QString &belt) const;
    bool cleared(const QString &belt) const;
    void markTaught(const QString &belt);
    void markCleared(const QString &belt);

    ReviewResult review(const QString &id, bool correct, int ms);
    int correctCount(const QString &id) const;
    int wrongCount(const QString &id) const;
    int mastery(const QString &id) const;
    bool seen(const QString &id) const;

    QStringList due(const QStringList &ids) const;
    QStringList weakest(const QStringList &ids, int limit) const;

    void recordSession(const QString &kind, const QString &title, int correct, int total, int avgMs, int xp);
    int streakCurrent() const;
    int streakBest() const;

    RankPlace rank() const;

    static QString defaultPath();

private:
    void load();
    void save();
    void ensure();
    void migrate();
    void touchStreak(const QString &today);
    QJsonObject item(const QString &id) const;
    void putItem(const QString &id, const QJsonObject &item);
    static int masteryOf(const QJsonObject &item);

    QString m_path;
    QString m_warning;
    QJsonObject m_doc;
};
