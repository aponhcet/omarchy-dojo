#include "store.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTime>
#include <QtMath>
#include <algorithm>

namespace {

QString today()
{
    return QDate::currentDate().toString(Qt::ISODate);
}

double round3(double value)
{
    return qRound(value * 1000.0) / 1000.0;
}

QJsonArray stringArray(const QJsonArray &array)
{
    QJsonArray out;
    for (const auto &value : array)
        if (value.isString())
            out.append(value.toString());
    return out;
}

bool containsString(const QJsonArray &array, const QString &needle)
{
    for (const auto &value : array)
        if (value.toString() == needle)
            return true;
    return false;
}

} // namespace

QString Store::defaultPath()
{
    // XDG_STATE_HOME, or ~/.local/state when it is unset. The file name stays
    // stable so an existing Dojo keeps its belts.
    QString root = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
    if (root.isEmpty())
        root = QDir::homePath() + QStringLiteral("/.local/state");
    return root + QStringLiteral("/omarchy-dojo/state.json");
}

Store::Store(QString path)
    : m_path(path.isEmpty() ? defaultPath() : path)
{
    ensure();
    load();
}

void Store::ensure()
{
    m_doc = QJsonObject{
        {"version", 2},
        {"xp", 0},
        {"streak", QJsonObject{{"current", 0}, {"best", 0}, {"last_active", ""}, {"days", QJsonArray{}}}},
        {"training", QJsonObject{{"taught", QJsonArray{}}, {"cleared", QJsonArray{}}, {"completed", QJsonArray{}}}},
        {"items", QJsonObject{}},
        {"sessions", QJsonArray{}},
    };
}

void Store::load()
{
    QFile file(m_path);
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly)) {
        m_warning = file.errorString();
        return;
    }
    const auto bytes = file.readAll();
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(bytes, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        QFile::rename(m_path, m_path + ".unreadable");
        m_warning = QStringLiteral("unreadable");
        return;
    }
    m_doc = document.object();
    if (!m_doc.contains("items"))
        m_doc.insert("items", QJsonObject{});
    if (!m_doc.contains("sessions"))
        m_doc.insert("sessions", QJsonArray{});
    if (!m_doc.contains("streak"))
        m_doc.insert("streak", QJsonObject{{"current", 0}, {"best", 0}, {"last_active", ""}, {"days", QJsonArray{}}});
    if (!m_doc.contains("training"))
        m_doc.insert("training", QJsonObject{});
    migrate();
}

void Store::migrate()
{
    if (m_doc.value("version").toInt(1) >= 2)
        return;
    auto training = m_doc.value("training").toObject();
    const auto completed = stringArray(training.value("completed").toArray());
    if (training.value("taught").toArray().isEmpty())
        training.insert("taught", completed);
    if (training.value("cleared").toArray().isEmpty())
        training.insert("cleared", completed);
    m_doc.insert("training", training);

    // A finished first belt and the old correct answers should still count,
    // without jumping someone who played one evening to the last rank.
    if (m_doc.value("xp").toInt() == 0) {
        int xp = completed.size() * 50;
        const auto items = m_doc.value("items").toObject();
        for (auto it = items.begin(); it != items.end(); ++it)
            xp += it.value().toObject().value("correct").toInt() * 5;
        m_doc.insert("xp", xp);
    }
    m_doc.insert("version", 2);
    save();
}

void Store::save()
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_warning = file.errorString();
        return;
    }
    file.write(QJsonDocument(m_doc).toJson(QJsonDocument::Indented));
    if (!file.commit())
        m_warning = file.errorString();
}

int Store::xp() const
{
    return m_doc.value("xp").toInt();
}

void Store::addXp(int amount)
{
    if (amount <= 0)
        return;
    m_doc.insert("xp", xp() + amount);
    save();
}

int Store::mastered() const
{
    int count = 0;
    const auto items = m_doc.value("items").toObject();
    for (auto it = items.begin(); it != items.end(); ++it)
        if (masteryOf(it.value().toObject()) >= 4)
            ++count;
    return count;
}

bool Store::taught(const QString &belt) const
{
    return containsString(m_doc.value("training").toObject().value("taught").toArray(), belt);
}

bool Store::cleared(const QString &belt) const
{
    return containsString(m_doc.value("training").toObject().value("cleared").toArray(), belt);
}

void Store::markTaught(const QString &belt)
{
    auto training = m_doc.value("training").toObject();
    auto taught = training.value("taught").toArray();
    if (!containsString(taught, belt))
        taught.append(belt);
    training.insert("taught", taught);
    m_doc.insert("training", training);
    save();
}

void Store::markCleared(const QString &belt)
{
    auto training = m_doc.value("training").toObject();
    auto cleared = training.value("cleared").toArray();
    if (!containsString(cleared, belt))
        cleared.append(belt);
    auto taught = training.value("taught").toArray();
    if (!containsString(taught, belt))
        taught.append(belt);
    training.insert("cleared", cleared);
    training.insert("taught", taught);
    training.insert("completed", cleared);
    m_doc.insert("training", training);
    save();
}

QJsonObject Store::item(const QString &id) const
{
    return m_doc.value("items").toObject().value(id).toObject();
}

void Store::putItem(const QString &id, const QJsonObject &value)
{
    auto items = m_doc.value("items").toObject();
    items.insert(id, value);
    m_doc.insert("items", items);
}

int Store::masteryOf(const QJsonObject &item)
{
    const int seen = item.value("seen").toInt();
    const int correct = item.value("correct").toInt();
    if (seen <= 0)
        return 0;
    if (correct <= 0)
        return 1;
    const double accuracy = double(correct) / double(seen);
    const int avg = item.value("avg_ms").toInt();
    const int streak = item.value("streak").toInt();
    if (seen >= 8 && accuracy >= 0.85 && avg > 0 && avg < 2500 && streak >= 3)
        return 5;
    if (seen >= 5 && accuracy >= 0.75 && (avg == 0 || avg < 4000))
        return 4;
    if (seen >= 3 && accuracy >= 0.5)
        return 3;
    return 2;
}

ReviewResult Store::review(const QString &id, bool correct, int ms)
{
    auto row = item(id);
    ms = std::max(0, ms);
    row.insert("seen", row.value("seen").toInt() + 1);
    ReviewResult result;
    result.correct = correct;
    if (correct) {
        row.insert("correct", row.value("correct").toInt() + 1);
        row.insert("streak", row.value("streak").toInt() + 1);
        auto samples = row.value("samples").toArray();
        samples.append(ms);
        while (samples.size() > 12)
            samples.removeAt(0);
        int sum = 0;
        for (const auto &sample : samples)
            sum += sample.toInt();
        row.insert("samples", samples);
        row.insert("avg_ms", samples.isEmpty() ? 0 : sum / samples.size());
        const int previousBest = row.value("best_ms").toInt();
        if (previousBest == 0 || ms < previousBest) {
            result.newBest = previousBest > 0;
            row.insert("best_ms", ms);
        }
        result.bestMs = row.value("best_ms").toInt();
        const int quality = ms < 1800 ? 5 : ms < 3200 ? 4 : 3;
        double ease = row.value("ease").toDouble(2.5);
        if (ease <= 0)
            ease = 2.5;
        ease = std::max(1.3, round3(ease + 0.1 - (5 - quality) * 0.08));
        row.insert("ease", ease);
        const int streak = row.value("streak").toInt();
        int interval = row.value("interval").toInt();
        if (streak == 1)
            interval = 1;
        else if (streak == 2)
            interval = 2;
        else
            interval = std::max(1, int(std::round(interval * ease)));
        row.insert("interval", interval);
    } else {
        row.insert("wrong", row.value("wrong").toInt() + 1);
        row.insert("streak", 0);
        double ease = row.value("ease").toDouble(2.5);
        if (ease <= 0)
            ease = 2.5;
        row.insert("ease", std::max(1.3, round3(ease - 0.2)));
        row.insert("interval", 0);
    }
    const int interval = row.value("interval").toInt();
    row.insert("due", QDate::currentDate().addDays(interval).toString(Qt::ISODate));
    row.insert("mastery", masteryOf(row));
    putItem(id, row);
    save();
    return result;
}

int Store::correctCount(const QString &id) const
{
    return item(id).value("correct").toInt();
}

int Store::wrongCount(const QString &id) const
{
    return item(id).value("wrong").toInt();
}

int Store::mastery(const QString &id) const
{
    const auto row = item(id);
    if (row.isEmpty())
        return 0;
    return row.contains("mastery") ? row.value("mastery").toInt() : masteryOf(row);
}

bool Store::seen(const QString &id) const
{
    return item(id).value("seen").toInt() > 0;
}

QStringList Store::due(const QStringList &ids) const
{
    const auto stamp = today();
    QStringList due;
    QStringList unseen;
    for (const auto &id : ids) {
        const auto row = item(id);
        if (row.value("seen").toInt() <= 0) {
            unseen.append(id);
            continue;
        }
        const auto when = row.value("due").toString();
        if (when.isEmpty() || when <= stamp)
            due.append(id);
    }
    due.append(unseen);
    return due;
}

QStringList Store::weakest(const QStringList &ids, int limit) const
{
    struct Scored {
        int mastery;
        double accuracy;
        int wrong;
        QString id;
    };
    QList<Scored> scored;
    for (const auto &id : ids) {
        const auto row = item(id);
        const int seen = row.value("seen").toInt();
        const int correct = row.value("correct").toInt();
        scored.append({masteryOf(row), seen ? double(correct) / seen : 0, row.value("wrong").toInt(), id});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        if (a.mastery != b.mastery)
            return a.mastery < b.mastery;
        if (a.accuracy != b.accuracy)
            return a.accuracy < b.accuracy;
        return a.wrong > b.wrong;
    });
    QStringList out;
    for (const auto &row : scored) {
        if (out.size() >= limit)
            break;
        out.append(row.id);
    }
    return out;
}

void Store::recordSession(const QString &kind, const QString &title, int correct, int total, int avgMs, int gained)
{
    auto sessions = m_doc.value("sessions").toArray();
    sessions.append(QJsonObject{
        {"type", kind},
        {"title", title},
        {"date", today()},
        {"time", QTime::currentTime().toString("HH:mm")},
        {"correct", correct},
        {"total", total},
        {"avg_ms", avgMs},
        {"xp", gained},
    });
    while (sessions.size() > 60)
        sessions.removeAt(0);
    m_doc.insert("sessions", sessions);
    touchStreak(today());
    save();
}

void Store::touchStreak(const QString &stamp)
{
    auto streak = m_doc.value("streak").toObject();
    auto days = streak.value("days").toArray();
    if (containsString(days, stamp)) {
        streak.insert("last_active", stamp);
        m_doc.insert("streak", streak);
        return;
    }
    const auto last = streak.value("last_active").toString();
    int gap = 99;
    if (!last.isEmpty()) {
        const auto previous = QDate::fromString(last, Qt::ISODate);
        if (previous.isValid())
            gap = previous.daysTo(QDate::fromString(stamp, Qt::ISODate));
    }
    int current = streak.value("current").toInt();
    if (gap == 1)
        current += 1;
    else if (gap != 0)
        current = 1;
    streak.insert("current", current);
    streak.insert("best", std::max(streak.value("best").toInt(), current));
    streak.insert("last_active", stamp);
    days.append(stamp);
    while (days.size() > 120)
        days.removeAt(0);
    streak.insert("days", days);
    m_doc.insert("streak", streak);
}

int Store::streakCurrent() const
{
    const auto streak = m_doc.value("streak").toObject();
    const auto last = streak.value("last_active").toString();
    if (last.isEmpty())
        return 0;
    const auto date = QDate::fromString(last, Qt::ISODate);
    if (!date.isValid() || date.daysTo(QDate::currentDate()) > 1)
        return 0;
    return streak.value("current").toInt();
}

int Store::streakBest() const
{
    return m_doc.value("streak").toObject().value("best").toInt();
}

RankPlace Store::rank() const
{
    const int points = xp();
    RankPlace place;
    const int count = int(std::size(kRanks));
    for (int i = 0; i < count; ++i) {
        if (points < kRanks[i].xp)
            break;
        place.index = i;
        place.floor = kRanks[i].xp;
    }
    if (place.index + 1 < count) {
        place.next = kRanks[place.index + 1].xp;
    } else {
        place.next = place.floor;
        place.top = true;
    }
    return place;
}
