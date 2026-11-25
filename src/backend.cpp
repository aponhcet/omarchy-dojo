#include "backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QProcess>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <algorithm>

namespace {

QString t(const char *source)
{
    const QString text = QCoreApplication::translate("Dojo", source);
    return text.isEmpty() ? QString::fromUtf8(source) : text;
}

QString tn(const char *source, int count)
{
    QString text = QCoreApplication::translate("Dojo", source, nullptr, count);
    if (text.isEmpty())
        text = QString::fromUtf8(source);
    if (text.contains("%n"))
        text.replace("%n", QLocale().toString(count));
    return text;
}

const BeltDef *findBelt(const QString &id)
{
    int count = 0;
    const auto *defs = beltDefs(&count);
    for (int i = 0; i < count; ++i)
        if (id == QLatin1String(defs[i].id))
            return defs + i;
    return nullptr;
}

QStringList visibleBelts(const QList<Bind> &binds)
{
    QStringList ids;
    int count = 0;
    const auto *defs = beltDefs(&count);
    for (int i = 0; i < count; ++i) {
        const auto id = QString::fromUtf8(defs[i].id);
        if (beltCount(binds, id) > 0)
            ids.append(id);
    }
    return ids;
}

void shuffleBinds(QList<Bind> &binds)
{
    std::shuffle(binds.begin(), binds.end(), *QRandomGenerator::global());
}

} // namespace

Backend::Backend(const QString &statePath, const QString &bindings, bool capture, const QString &hyprctl, QObject *parent)
    : QObject(parent),
      m_store(statePath),
      m_grab(hyprctl.isEmpty() && capture ? QStandardPaths::findExecutable("hyprctl") : hyprctl, capture),
      m_capture(capture)
{
    m_flash.setSingleShot(true);
    connect(&m_flash, &QTimer::timeout, this, &Backend::onFlash);
    if (bindings.isEmpty())
        loadLiveBindings();
    else
        m_binds = loadBinds(bindings);
    if (m_binds.isEmpty() && m_error.isEmpty())
        m_error = t("Omarchy printed no keybindings, so there is nothing to teach.");
    rebuildRows();
    selectInitial();
    rebuildAtlas();
}

void Backend::loadLiveBindings()
{
    const auto program = QStandardPaths::findExecutable("omarchy");
    if (program.isEmpty()) {
        m_error = t("Omarchy is not on PATH, so there is nothing to teach.");
        return;
    }
    QProcess process;
    process.start(program, {"menu", "keybindings", "--print"});
    if (!process.waitForFinished(8000) || process.exitStatus() != QProcess::NormalExit) {
        m_error = t("Omarchy did not answer with the keybindings.");
        return;
    }
    m_binds = loadBinds(QString::fromUtf8(process.readAllStandardOutput()));
}

void Backend::touch()
{
    ++m_generation;
    emit updated();
}

void Backend::setTyping(bool typing)
{
    if (m_typing == typing)
        return;
    m_typing = typing;
    touch();
}

void Backend::retranslate()
{
    touch();
}

QString Backend::rankName() const
{
    return t(kRanks[m_store.rank().index].name);
}

QString Backend::rankTagline() const
{
    return t(kRanks[m_store.rank().index].tagline);
}

bool Backend::rankTop() const
{
    return m_store.rank().top;
}

int Backend::xpNext() const
{
    return m_store.rank().next;
}

double Backend::rankFraction() const
{
    const auto place = m_store.rank();
    if (place.top || place.next <= place.floor)
        return 1;
    return qBound(0.0, double(m_store.xp() - place.floor) / double(place.next - place.floor), 1.0);
}

QString Backend::streakLine() const
{
    const int days = m_store.streakCurrent();
    if (days <= 0)
        return t("No streak yet. One session today lights it.");
    if (days == 1)
        return t("Day one. Super is a habit waiting.");
    if (days < 7)
        return tn("%n days. The hands are taking notes.", days);
    return tn("%n days of Super.", days);
}

QString Backend::beltTitle(const QString &id) const
{
    if (const auto *belt = findBelt(id))
        return t(belt->title);
    return id;
}

QString Backend::beltBlurb(const QString &id) const
{
    if (const auto *belt = findBelt(id))
        return t(belt->blurb);
    return {};
}

void Backend::rebuildRows()
{
    m_rows.clear();
    const auto order = visibleBelts(m_binds);
    QString previous;
    for (const auto &id : order) {
        Row row;
        row.kind = "belt";
        row.id = id;
        const bool locked = !previous.isEmpty() && !m_store.cleared(previous);
        if (locked)
            row.state = "locked";
        else if (!m_store.taught(id))
            row.state = "teach";
        else if (!m_store.cleared(id))
            row.state = "practice";
        else
            row.state = "cleared";
        m_rows.append(row);
        previous = id;
    }
    if (!order.isEmpty() && m_store.cleared(order.first())) {
        for (const char *id : {"daily", "weak", "speed", "boss"})
            m_rows.append(Row{"spar", QString::fromUtf8(id), "spar"});
    }
    if (m_selected >= m_rows.size())
        m_selected = 0;
}

void Backend::selectInitial()
{
    m_selected = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const auto &row = m_rows.at(i);
        if (row.state == "locked" || row.state == "cleared")
            continue;
        m_selected = i;
        break;
    }
}

QVariantMap Backend::viewFor(const Row &row) const
{
    QVariantMap map;
    map.insert("kind", row.kind);
    map.insert("id", row.id);
    map.insert("state", row.state);
    if (row.kind == "spar") {
        map.insert("kicker", t("Sparring"));
        if (row.id == "daily") {
            map.insert("title", t("Daily"));
            map.insert("detail", t("Due binds from the belts you have cleared."));
            map.insert("action", t("Start"));
        } else if (row.id == "weak") {
            map.insert("title", t("Weak spots"));
            map.insert("detail", t("The ones that still miss."));
            map.insert("action", t("Start"));
        } else if (row.id == "speed") {
            map.insert("title", t("Speed run"));
            map.insert("detail", t("A short race. One try each."));
            map.insert("action", t("Start"));
        } else {
            map.insert("title", t("Boss fight"));
            map.insert("detail", t("Three lives. Spend them slowly."));
            map.insert("action", t("Start"));
        }
        return map;
    }

    map.insert("title", beltTitle(row.id));
    if (row.state == "locked") {
        QString previousTitle;
        const auto order = visibleBelts(m_binds);
        const int index = order.indexOf(row.id);
        if (index > 0)
            previousTitle = beltTitle(order.at(index - 1));
        map.insert("kicker", t("Locked"));
        map.insert("detail", t("Opens after %1.").arg(previousTitle));
        map.insert("action", QString());
    } else if (row.state == "teach") {
        map.insert("kicker", t("Teach"));
        map.insert("detail", beltBlurb(row.id));
        map.insert("action", t("Teach"));
    } else if (row.state == "practice") {
        map.insert("kicker", t("Practice"));
        map.insert("detail", t("From memory. The keys stay hidden."));
        map.insert("action", t("Practice"));
    } else {
        map.insert("kicker", t("Cleared"));
        map.insert("detail", t("Cleared. Practice keeps it sharp."));
        map.insert("action", t("Practice again"));
    }
    return map;
}

QVariantMap Backend::rowAt(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_rows.size())
        return map;
    map = viewFor(m_rows.at(index));
    map.insert("selected", index == m_selected);
    return map;
}

QString Backend::heroKicker() const
{
    if (!m_error.isEmpty())
        return t("Dojo");
    if (m_rows.isEmpty())
        return t("Dojo");
    return rowAt(m_selected).value("kicker").toString();
}

QString Backend::heroTitle() const
{
    if (!m_error.isEmpty())
        return m_error;
    if (m_rows.isEmpty())
        return t("Nothing to teach");
    return rowAt(m_selected).value("title").toString();
}

QString Backend::heroDetail() const
{
    if (!m_error.isEmpty())
        return {};
    if (m_rows.isEmpty())
        return {};
    return rowAt(m_selected).value("detail").toString();
}

QString Backend::heroAction() const
{
    if (m_rows.isEmpty())
        return {};
    return rowAt(m_selected).value("action").toString();
}

void Backend::move(int delta)
{
    if (m_help) {
        toggleHelp();
        return;
    }
    if (m_page == "atlas") {
        if (m_atlas.isEmpty())
            return;
        m_atlasSelected = (m_atlasSelected + delta + m_atlas.size()) % m_atlas.size();
        touch();
        return;
    }
    if (m_page != "home" || m_rows.isEmpty())
        return;
    int index = m_selected;
    for (int step = 0; step < m_rows.size(); ++step) {
        index = (index + delta + m_rows.size()) % m_rows.size();
        if (m_rows.at(index).state != "locked") {
            m_selected = index;
            m_status.clear();
            touch();
            return;
        }
    }
}

void Backend::choose(int index)
{
    if (index < 0 || index >= m_rows.size())
        return;
    m_selected = index;
    if (m_rows.at(index).state == "locked") {
        m_status = rowAt(index).value("detail").toString();
        touch();
        return;
    }
    m_status.clear();
    startRow(m_rows.at(index));
}

void Backend::primaryAction()
{
    if (m_help) {
        toggleHelp();
        return;
    }
    if (m_confirm == "quit") {
        finish(false);
        emit quitRequested();
        return;
    }
    if (m_confirm == "leave") {
        finish(false);
        return;
    }
    if (m_page == "home") {
        if (m_selected >= 0 && m_selected < m_rows.size())
            choose(m_selected);
        return;
    }
    if (m_page == "result") {
        const auto next = m_result.next;
        const auto belt = m_result.nextBelt.isEmpty() ? m_result.belt : m_result.nextBelt;
        const auto mode = m_result.mode;
        if (next == "practice")
            begin("practice", m_result.belt, beltTitle(m_result.belt), beltBinds(m_binds, m_result.belt), 2, -1);
        else if (next == "teach")
            begin("teach", belt, beltTitle(belt), beltBinds(m_binds, belt), 1, -1);
        else if (next == "retry")
            begin("practice", m_result.belt, beltTitle(m_result.belt), beltBinds(m_binds, m_result.belt), 2, -1);
        else if (next == "again") {
            Row row{"spar", mode, "spar"};
            if (mode == "drill")
                showHome();
            else
                startRow(row);
        } else {
            showHome();
        }
        return;
    }
    if (m_page == "atlas")
        chooseAtlas(m_atlasSelected);
}

bool Backend::startRow(const Row &row)
{
    if (row.kind == "belt") {
        const auto cards = beltBinds(m_binds, row.id);
        if (cards.isEmpty())
            return false;
        if (!m_store.taught(row.id))
            return begin("teach", row.id, beltTitle(row.id), cards, 1, -1);
        auto practice = cards;
        shuffleBinds(practice);
        return begin("practice", row.id, beltTitle(row.id), practice, 2, -1);
    }

    auto cards = pool();
    if (cards.isEmpty()) {
        m_status = t("Clear a belt before sparring.");
        touch();
        return false;
    }
    const auto ids = [&cards] {
        QStringList out;
        for (const auto &bind : cards)
            out.append(bind.id);
        return out;
    }();
    QList<Bind> chosen;
    int tries = 2;
    int lives = -1;
    QString title;
    if (row.id == "daily") {
        auto pick = bindsForIds(m_store.due(ids));
        if (pick.isEmpty())
            pick = bindsForIds(m_store.weakest(ids, 8));
        chosen = pick.mid(0, 8);
        title = t("Daily");
    } else if (row.id == "weak") {
        chosen = bindsForIds(m_store.weakest(ids, 8));
        title = t("Weak spots");
    } else if (row.id == "speed") {
        chosen = cards;
        shuffleBinds(chosen);
        chosen = chosen.mid(0, 10);
        tries = 1;
        title = t("Speed run");
    } else if (row.id == "boss") {
        chosen = cards;
        shuffleBinds(chosen);
        chosen = chosen.mid(0, 10);
        tries = 1;
        lives = 3;
        title = t("Boss fight");
    } else {
        return false;
    }
    if (chosen.isEmpty())
        return false;
    if (row.id == "daily" || row.id == "weak")
        shuffleBinds(chosen);
    return begin(row.id, {}, title, chosen, tries, lives);
}

QList<Bind> Backend::pool() const
{
    QList<Bind> cards;
    for (const auto &bind : m_binds) {
        if (bind.practiceable && !bind.belt.isEmpty() && m_store.cleared(bind.belt))
            cards.append(bind);
    }
    return cards;
}

QList<Bind> Backend::bindsForIds(const QStringList &ids) const
{
    QList<Bind> cards;
    for (const auto &id : ids) {
        for (const auto &bind : m_binds) {
            if (bind.id == id) {
                cards.append(bind);
                break;
            }
        }
    }
    return cards;
}

QString Backend::grabError() const
{
    return t("Hyprland did not open the practice submap, so your real shortcuts would still fire. Super+Shift+Escape resets it.");
}

bool Backend::begin(const QString &mode, const QString &belt, const QString &title, QList<Bind> cards, int tries, int lives)
{
    if (cards.isEmpty())
        return false;
    // Teach stays in lesson order. Practice and sparring are shuffled so the
    // position on the list is not the thing being memorized.
    if (mode != "teach" && mode != "drill")
        shuffleBinds(cards);
    QString error;
    if (!m_grab.enter(&error)) {
        m_status = grabError();
        m_page = "home";
        touch();
        return false;
    }
    m_round = Round{};
    m_round.active = true;
    m_round.mode = mode;
    m_round.belt = belt;
    m_round.title = title;
    m_round.cards = cards;
    m_round.triesLeft = tries;
    m_round.lives = lives;
    m_round.rankIndex = m_store.rank().index;
    m_confirm.clear();
    m_status.clear();
    m_help = false;
    m_page = "session";
    startCard();
    touch();
    return true;
}

const Bind *Backend::current() const
{
    if (!m_round.active || m_round.index < 0 || m_round.index >= m_round.cards.size())
        return nullptr;
    return &m_round.cards.at(m_round.index);
}

void Backend::startCard()
{
    // Misses still allowed on this card. Teaching never runs out.
    if (m_round.mode == "speed" || m_round.mode == "boss")
        m_round.triesLeft = 1;
    else if (m_round.mode != "teach")
        m_round.triesLeft = 2;
    m_round.showKeys = m_round.mode == "teach";
    m_round.coach.clear();
    m_round.tone = "idle";
    m_clock.restart();
}

QString Backend::sessionKicker() const
{
    if (m_round.mode == "teach")
        return t("Teach");
    if (m_round.mode == "practice")
        return t("Practice");
    if (m_round.mode == "daily")
        return t("Daily");
    if (m_round.mode == "weak")
        return t("Weak spots");
    if (m_round.mode == "speed")
        return t("Speed run");
    if (m_round.mode == "boss")
        return t("Boss fight");
    return t("Drill");
}

QString Backend::sessionTitle() const
{
    return m_round.title;
}

int Backend::sessionIndex() const
{
    return m_round.cards.isEmpty() ? 0 : m_round.index + 1;
}

int Backend::sessionTotal() const
{
    return m_round.cards.size();
}

double Backend::sessionProgress() const
{
    if (m_round.cards.isEmpty())
        return 0;
    return qBound(0.0, double(m_round.index) / double(m_round.cards.size()), 1.0);
}

QString Backend::sessionAction() const
{
    const auto *bind = current();
    return bind ? bind->description : QString();
}

QString Backend::sessionWhen() const
{
    if (m_confirm == "leave")
        return t("Leave this session? Enter leaves. Esc stays.");
    if (m_confirm == "quit")
        return t("Quit Dojo? Enter quits. Esc stays.");
    const auto *bind = current();
    if (!bind)
        return {};
    if (m_round.mode == "teach")
        return teachLine(bind->description);
    return t("From memory.");
}

bool Backend::showKeys() const
{
    return m_round.showKeys;
}

QStringList Backend::keyParts() const
{
    if (!m_round.showKeys)
        return {QStringLiteral("?")};
    const auto *bind = current();
    if (!bind || bind->combos.isEmpty())
        return {};
    return bind->primary().parts();
}

QStringList Backend::altKeyParts() const
{
    if (!m_round.showKeys)
        return {};
    const auto *bind = current();
    if (!bind || bind->combos.size() < 2)
        return {};
    return bind->combos.at(1).parts();
}

QString Backend::keyLine() const
{
    const auto *bind = current();
    if (!bind)
        return {};
    QStringList labels;
    for (const auto &combo : bind->combos)
        labels.append(combo.label());
    return labels.join("  /  ");
}

QString Backend::coach() const
{
    return m_round.coach;
}

QString Backend::coachTone() const
{
    return m_round.tone;
}

int Backend::combo() const
{
    return m_round.combo;
}

int Backend::lives() const
{
    return m_round.lives;
}

QString Backend::sessionHint() const
{
    if (m_confirm == "leave")
        return t("Enter leaves. Esc stays.");
    if (m_confirm == "quit")
        return t("Enter quits. Esc stays.");
    if (m_round.mode == "teach")
        return t("Press the bind. Esc leaves. ? help. Q quits.");
    return t("From memory. H reveals. Tab skips. Esc leaves.");
}

QString Backend::hitLine()
{
    static const char *lines[] = {
        "Clean.",
        "That's the one.",
        "The mouse can watch.",
        "Again. Faster.",
        "Hands, not eyes.",
    };
    return t(lines[QRandomGenerator::global()->bounded(int(std::size(lines)))]);
}

QString Backend::missLine()
{
    static const char *lines[] = {
        "Not that one.",
        "Look, then press it.",
        "The modifiers count.",
        "Shake it off.",
    };
    return t(lines[QRandomGenerator::global()->bounded(int(std::size(lines)))]);
}

void Backend::judge(const Combo &got)
{
    const auto *bind = current();
    if (!bind || !got.valid() || m_flash.isActive() || !m_confirm.isEmpty())
        return;
    const int ms = int(m_clock.elapsed());
    if (bind->matches(got)) {
        m_round.hits += 1;
        m_round.combo += 1;
        m_round.bestCombo = std::max(m_round.bestCombo, m_round.combo);
        m_round.hitMs.append(ms);
        int gained = m_round.mode == "teach" ? 10 : 15;
        if (m_round.mode != "teach" && ms < 1500)
            gained += 5;
        if (m_round.combo > 0 && m_round.combo % 3 == 0)
            gained += 10;
        m_round.xp += gained;
        m_store.addXp(gained);
        ReviewResult review;
        if (m_round.mode != "teach")
            review = m_store.review(bind->id, true, ms);
        m_round.coach = hitLine();
        if (review.newBest)
            m_round.coach += " " + t("New best.");
        m_round.tone = "hit";
        m_round.showKeys = true;
        m_round.after = "advance";
        m_flash.start(640);
        touch();
        return;
    }

    m_round.combo = 0;
    m_round.misses += 1;
    m_round.coach = missLine();
    m_round.tone = "miss";
    m_round.showKeys = true;
    if (m_round.mode != "teach")
        m_store.review(bind->id, false, ms);
    if (m_round.mode == "teach") {
        m_round.after = "retry";
        m_flash.start(800);
        touch();
        return;
    }
    if (m_round.lives >= 0)
        m_round.lives -= 1;
    m_round.triesLeft -= 1;
    if (m_round.lives == 0)
        m_round.after = "finish";
    else if (m_round.triesLeft > 0)
        m_round.after = "retry";
    else
        m_round.after = "advance";
    m_flash.start(m_round.after == "retry" ? 800 : 1000);
    touch();
}

void Backend::onFlash()
{
    if (m_round.after == "finish") {
        finish(true);
        return;
    }
    if (m_round.after == "advance") {
        advance();
        return;
    }
    m_round.coach.clear();
    m_round.tone = "idle";
    if (m_round.mode == "teach")
        m_round.showKeys = true;
    m_clock.restart();
    touch();
}

void Backend::advance()
{
    m_round.index += 1;
    if (m_round.index >= m_round.cards.size()) {
        finish(true);
        return;
    }
    startCard();
    touch();
}

void Backend::finish(bool completed)
{
    m_flash.stop();
    m_grab.leave();
    const bool attempted = m_round.hits + m_round.misses > 0;
    bool cleared = false;
    if (completed && m_round.mode == "teach" && !m_round.belt.isEmpty())
        m_store.markTaught(m_round.belt);
    if (completed && m_round.mode == "practice" && !m_round.belt.isEmpty() && m_round.cards.size() > 0
        && m_round.hits * 10 >= m_round.cards.size() * 7) {
        m_store.markCleared(m_round.belt);
        cleared = true;
    }
    int avg = 0;
    if (!m_round.hitMs.isEmpty()) {
        int sum = 0;
        for (int ms : m_round.hitMs)
            sum += ms;
        avg = sum / m_round.hitMs.size();
    }
    if (attempted)
        m_store.recordSession(m_round.mode, m_round.title, m_round.hits, m_round.cards.size(), avg, m_round.xp);

    m_result = Result{};
    m_result.mode = m_round.mode;
    m_result.belt = m_round.belt;
    m_result.title = m_round.title;
    m_result.hits = m_round.hits;
    m_result.total = m_round.cards.size();
    m_result.misses = m_round.misses;
    m_result.avgMs = avg;
    m_result.xp = m_round.xp;
    m_result.bestCombo = m_round.bestCombo;
    m_result.cleared = cleared;
    m_result.completed = completed;
    m_result.rankIndex = m_store.rank().index;
    m_result.rankUp = m_store.rank().index > m_round.rankIndex;
    m_result.next = "home";
    if (!completed) {
        m_result.next = "home";
    } else if (m_round.mode == "teach") {
        m_result.next = "practice";
    } else if (m_round.mode == "practice" && cleared) {
        const auto order = visibleBelts(m_binds);
        const int index = order.indexOf(m_round.belt);
        if (index >= 0 && index + 1 < order.size()) {
            m_result.next = "teach";
            m_result.nextBelt = order.at(index + 1);
        }
    } else if (m_round.mode == "practice") {
        m_result.next = "retry";
    } else if (m_round.mode == "drill") {
        m_result.next = "home";
    } else if (!m_round.mode.isEmpty()) {
        m_result.next = "again";
    }

    m_round = Round{};
    m_confirm.clear();
    m_page = attempted || completed ? "result" : "home";
    if (m_page == "home") {
        rebuildRows();
        selectInitial();
    }
    touch();
}

void Backend::leaveRound()
{
    m_flash.stop();
    m_grab.leave();
    m_confirm.clear();
}

void Backend::flush()
{
    if (!m_flash.isActive())
        return;
    m_flash.stop();
    onFlash();
}

void Backend::submitCombo(const QString &id)
{
    auto text = id;
    text.replace('+', " + ");
    judge(parseCombo(text));
}

void Backend::submitKey(int key, int modifiers)
{
    judge(comboFromQt(key, modifiers));
}

QString Backend::solutionId() const
{
    const auto *bind = current();
    return bind ? bind->primary().id() : QString();
}

QString Backend::cardId() const
{
    const auto *bind = current();
    return bind ? bind->id : QString();
}

int Backend::itemWrong(const QString &id) const
{
    return m_store.wrongCount(id);
}

QStringList Backend::sessionBelts() const
{
    QStringList belts;
    for (const auto &bind : m_round.cards)
        belts.append(bind.belt);
    return belts;
}

void Backend::hint()
{
    if (m_page != "session" || m_round.mode == "teach" || !m_confirm.isEmpty())
        return;
    m_round.showKeys = true;
    touch();
}

void Backend::skip()
{
    if (m_page != "session" || m_round.mode == "teach" || m_flash.isActive() || !m_confirm.isEmpty())
        return;
    const auto *bind = current();
    if (!bind)
        return;
    m_store.review(bind->id, false, 0);
    m_round.misses += 1;
    m_round.combo = 0;
    if (m_round.lives >= 0) {
        m_round.lives -= 1;
        if (m_round.lives == 0) {
            finish(true);
            return;
        }
    }
    advance();
}

void Backend::back()
{
    if (m_help) {
        toggleHelp();
        return;
    }
    if (m_page == "session") {
        if (m_confirm == "leave") {
            m_confirm.clear();
            touch();
            return;
        }
        if (m_confirm == "quit") {
            m_confirm.clear();
            touch();
            return;
        }
        m_confirm = "leave";
        touch();
        return;
    }
    if (m_page == "atlas") {
        if (!m_query.isEmpty()) {
            setAtlasQuery({});
            return;
        }
        showHome();
        return;
    }
    if (m_page == "result")
        showHome();
}

void Backend::askQuit()
{
    if (m_help) {
        toggleHelp();
        return;
    }
    if (m_page == "session") {
        m_confirm = "quit";
        touch();
        return;
    }
    leaveRound();
    emit quitRequested();
}

void Backend::toggleHelp()
{
    m_help = !m_help;
    touch();
}

void Backend::showHome()
{
    m_grab.leave();
    m_round = Round{};
    m_confirm.clear();
    m_page = "home";
    m_typing = false;
    rebuildRows();
    selectInitial();
    touch();
}

void Backend::showAtlas()
{
    if (m_page == "session")
        return;
    m_page = "atlas";
    m_confirm.clear();
    rebuildAtlas();
    touch();
}

void Backend::setAtlasQuery(const QString &query)
{
    m_query = query;
    rebuildAtlas();
    touch();
}

void Backend::setFilter(const QString &id)
{
    if (!atlasFilters().contains(id) || id == m_filter)
        return;
    m_filter = id;
    rebuildAtlas();
    touch();
}

void Backend::cycleFilter(int delta)
{
    if (m_page != "atlas")
        return;
    const auto filters = atlasFilters();
    int index = filters.indexOf(m_filter);
    if (index < 0)
        index = 0;
    index = (index + delta + filters.size()) % filters.size();
    m_filter = filters.at(index);
    rebuildAtlas();
    touch();
}

QString Backend::filterLabel(const QString &id) const
{
    if (id == "all")
        return t("All");
    if (id == "other")
        return t("Other");
    return beltTitle(id);
}

QStringList Backend::atlasFilters() const
{
    QStringList filters = {QStringLiteral("all")};
    int count = 0;
    const auto *defs = beltDefs(&count);
    for (int i = 0; i < count; ++i)
        filters.append(QString::fromUtf8(defs[i].id));
    filters.append(QStringLiteral("other"));
    return filters;
}

void Backend::rebuildAtlas()
{
    const auto query = m_query.simplified().toLower();
    m_atlas.clear();
    for (int i = 0; i < m_binds.size(); ++i) {
        const auto &bind = m_binds.at(i);
        if (m_filter == "other" && !bind.belt.isEmpty())
            continue;
        if (m_filter != "all" && m_filter != "other" && bind.belt != m_filter)
            continue;
        if (!query.isEmpty()) {
            QString hay = bind.description + " " + bind.id + " " + bind.belt;
            for (const auto &combo : bind.combos)
                hay += " " + combo.label();
            if (!hay.toLower().contains(query))
                continue;
        }
        m_atlas.append(i);
    }
    if (m_atlasSelected >= m_atlas.size())
        m_atlasSelected = 0;
}

QVariantMap Backend::atlasAt(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_atlas.size())
        return map;
    const auto &bind = m_binds.at(m_atlas.at(index));
    QStringList labels;
    for (const auto &combo : bind.combos)
        labels.append(combo.label());
    QString dots;
    const int mastery = m_store.mastery(bind.id);
    const bool seen = m_store.seen(bind.id);
    for (int i = 0; i < 5; ++i)
        dots += seen && i < mastery ? QStringLiteral("●") : QStringLiteral("○");
    map.insert("description", bind.description);
    map.insert("keys", labels.join("   /   "));
    map.insert("dots", dots);
    map.insert("belt", bind.belt.isEmpty() ? t("Other") : beltTitle(bind.belt));
    map.insert("practiceable", bind.practiceable);
    map.insert("selected", index == m_atlasSelected);
    return map;
}

void Backend::chooseAtlas(int index)
{
    if (index < 0 || index >= m_atlas.size())
        return;
    m_atlasSelected = index;
    const auto &bind = m_binds.at(m_atlas.at(index));
    if (!bind.practiceable) {
        m_status = t("That bind uses a mouse or a media key, so the dojo cannot drill it.");
        touch();
        return;
    }
    QList<Bind> cards;
    for (int i = 0; i < 4; ++i)
        cards.append(bind);
    begin("drill", bind.belt, bind.description, cards, 2, -1);
}

QString Backend::resultKicker() const
{
    if (!m_result.completed)
        return t("Left early");
    if (m_result.mode == "teach")
        return t("Lesson complete");
    if (m_result.cleared)
        return t("Belt cleared");
    if (m_result.mode == "practice")
        return t("Not yet");
    return t("Session");
}

QString Backend::resultDetail() const
{
    if (!m_result.completed)
        return t("Nothing was cleared. The belt stays put.");
    // A rank and the next step can land on the same card. Say both, so the
    // reward does not hide what just unlocked.
    QString detail;
    auto add = [&detail](const QString &line) {
        if (line.isEmpty())
            return;
        if (!detail.isEmpty())
            detail += QLatin1Char(' ');
        detail += line;
    };
    if (m_result.rankUp)
        add(t("New rank: %1.").arg(t(kRanks[qBound(0, m_result.rankIndex, int(std::size(kRanks)) - 1)].name)));
    if (m_result.mode == "teach")
        add(t("You have seen them. Next they hide."));
    else if (m_result.cleared && !m_result.nextBelt.isEmpty())
        add(t("%1 is open.").arg(beltTitle(m_result.nextBelt)));
    else if (m_result.cleared)
        add(t("The path is clear. Spar whenever you want."));
    else if (m_result.mode == "practice")
        add(t("Most of them have to land before the next belt opens."));
    else if (m_result.bestCombo >= 3)
        add(tn("%n in a row.", m_result.bestCombo));
    else if (detail.isEmpty())
        add(streakLine());
    return detail;
}

QString Backend::resultAction() const
{
    if (m_result.next == "practice")
        return t("Practice");
    if (m_result.next == "teach")
        return t("Teach %1").arg(beltTitle(m_result.nextBelt));
    if (m_result.next == "retry")
        return t("Try again");
    if (m_result.next == "again")
        return t("Again");
    return t("Home");
}

QString Backend::resultRank() const
{
    return t(kRanks[qBound(0, m_result.rankIndex, int(std::size(kRanks)) - 1)].name);
}

bool Backend::handleKey(const QKeyEvent *event, bool press)
{
    if (!event)
        return false;
    const bool inSession = m_page == "session";
    if (!press) {
        // Releases during a round must not reach the text field or a shortcut.
        return inSession;
    }
    if (m_help) {
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Question
            || (event->key() == Qt::Key_Slash && event->modifiers() & Qt::ShiftModifier) || event->key() == Qt::Key_Space
            || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            toggleHelp();
        return true;
    }
    if (!inSession)
        return false;
    if (event->isAutoRepeat())
        return true;

    const auto bare = [event] {
        const auto mods = event->modifiers();
        return !(mods & (Qt::MetaModifier | Qt::ControlModifier | Qt::AltModifier));
    };

    if (!m_confirm.isEmpty()) {
        if (bare() && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
            primaryAction();
            return true;
        }
        if (bare() && event->key() == Qt::Key_Escape) {
            back();
            return true;
        }
        return true;
    }

    if (bare() && event->key() == Qt::Key_Escape) {
        back();
        return true;
    }
    if (bare() && event->key() == Qt::Key_Q) {
        askQuit();
        return true;
    }
    if (bare() && (event->key() == Qt::Key_Question || (event->key() == Qt::Key_Slash && event->modifiers() & Qt::ShiftModifier))) {
        toggleHelp();
        return true;
    }
    if (bare() && event->key() == Qt::Key_H) {
        hint();
        return true;
    }
    if (bare() && event->key() == Qt::Key_Tab) {
        skip();
        return true;
    }
    // Bare navigation keys belong to the window, not to a missed bind.
    if (bare() && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
                   || event->key() == Qt::Key_Up || event->key() == Qt::Key_Down || event->key() == Qt::Key_Left
                   || event->key() == Qt::Key_Right))
        return true;
    if (m_flash.isActive())
        return true;

    const auto got = comboFromQt(event->key(), int(event->modifiers()));
    if (!got.valid())
        return true;
    judge(got);
    return true;
}

QByteArray Backend::reportJson() const
{
    QJsonArray belts;
    int count = 0;
    const auto *defs = beltDefs(&count);
    for (int i = 0; i < count; ++i) {
        const auto id = QString::fromUtf8(defs[i].id);
        const int n = beltCount(m_binds, id);
        if (n == 0)
            continue;
        belts.append(QJsonObject{
            {"id", id},
            {"title", QString::fromUtf8(defs[i].title)},
            {"count", n},
            {"taught", m_store.taught(id)},
            {"cleared", m_store.cleared(id)},
        });
    }
    QJsonObject root{
        {"binds", m_binds.size()},
        {"error", m_error},
        {"warning", m_store.warning()},
        {"xp", m_store.xp()},
        {"rank", QString::fromUtf8(kRanks[m_store.rank().index].name)},
        {"belts", belts},
    };
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool Backend::catalogOk() const
{
    return m_error.isEmpty() && beltCount(m_binds, "essentials") >= 5;
}
