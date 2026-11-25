#include "catalog.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace {

const char *kEssentials[] = {
    "omarchy menu", "terminal", "browser", "close window", "keybindings", "file manager", "system menu", nullptr,
};
const char *kWindows[] = {
    "full screen", "full width", "toggle window floating/tiling", "toggle window split",
    "pop window out", "toggle transparency on a window", nullptr,
};
const char *kWorkspaces[] = {
    "switch to workspace 1", "switch to workspace 2", "switch to workspace 3",
    "next workspace", "previous workspace", "former workspace",
    "move window to workspace 1", "move window to workspace 2", nullptr,
};
const char *kFocus[] = {
    "focus on left window", "focus on right window", "focus on above window", "focus on below window",
    "focus on next window", "focus on previous window",
    "swap window to the left", "swap window to the right", nullptr,
};
const char *kCapture[] = {
    "screenshot", "screenrecording", "color picker", "extract text", "clipboard manager", nullptr,
};
const char *kSystem[] = {
    "lock system", "theme menu", "toggle top bar", "toggle locking on idle", "power menu", nullptr,
};
const char *kEveryday[] = {
    "editor", "universal copy", "universal paste", "universal cut", "emojis", "email", "music", "calculator", nullptr,
};
const char *kSignal[] = {
    "dismiss last notification", "dismiss all notifications", "toggle silencing notifications",
    "open notification history", "toggle scratchpad", "move window to scratchpad", "set reminder", nullptr,
};

const BeltDef kBelts[] = {
    {"essentials", "First Super", "Seven binds. Then the mouse can sit.", 7, kEssentials},
    {"windows", "Windows", "Float, split, and fill the screen.", 6, kWindows},
    {"workspaces", "Workspaces", "Three desks, then carry a window with you.", 8, kWorkspaces},
    {"focus", "Focus", "Point with Super, not the pointer.", 8, kFocus},
    {"capture", "Capture", "Grab what is on the screen.", 5, kCapture},
    {"system", "System", "Lock, theme, the bar, and power.", 5, kSystem},
    {"everyday", "Everyday", "Copy, paste, and the daily apps.", 8, kEveryday},
    {"signal", "Signal", "Notifications, and a pocket for windows.", 7, kSignal},
};

QString t(const char *source)
{
    const QString text = QCoreApplication::translate("Dojo", source);
    return text.isEmpty() ? QString::fromUtf8(source) : text;
}

bool needleHits(const QString &needle, const QString &description)
{
    const int start = description.indexOf(needle);
    if (start < 0)
        return false;
    const int end = start + needle.size();
    // "workspace 1" must not swallow "workspace 10".
    if (!needle.isEmpty() && needle.back().isDigit() && end < description.size() && description.at(end).isDigit())
        return false;
    return true;
}

int pickBind(const QList<Bind> &binds, const QSet<int> &used, const QString &needle)
{
    int fuzzy = -1;
    for (int i = 0; i < binds.size(); ++i) {
        if (used.contains(i) || !binds.at(i).practiceable)
            continue;
        const auto description = binds.at(i).description.toLower();
        if (description == needle)
            return i;
        if (fuzzy < 0 && needleHits(needle, description))
            fuzzy = i;
    }
    return fuzzy;
}

} // namespace

const BeltDef *beltDefs(int *count)
{
    if (count)
        *count = int(std::size(kBelts));
    return kBelts;
}

bool Bind::matches(const Combo &got) const
{
    for (const auto &combo : combos)
        if (combosMatch(combo, got))
            return true;
    return false;
}

QList<Bind> loadBinds(const QString &text)
{
    static const QSet<QString> skip = {"shortcut dojo", "leave dojo practice"};
    QList<Bind> binds;
    QSet<QString> seen;
    const auto lines = text.split('\n');
    for (const auto &line : lines) {
        const int arrow = line.indexOf(QChar(0x2192));
        if (arrow < 0)
            continue;
        const auto description = line.mid(arrow + 1).simplified();
        if (description.isEmpty() || skip.contains(description.toLower()))
            continue;
        const auto combos = parseAlternatives(line.left(arrow));
        if (combos.isEmpty())
            continue;

        Bind bind;
        bind.combos = combos;
        bind.description = description;
        for (const auto &combo : combos) {
            if (isPracticeableKey(combo.key)) {
                bind.practiceable = true;
                break;
            }
        }
        bind.id = combos.first().id() + "|" + description.toLower();
        if (seen.contains(bind.id))
            continue;
        seen.insert(bind.id);
        binds.append(bind);
    }

    // Needle order is the lesson order. The print order of the config is not
    // a curriculum, and "workspace 1" has to come before the other desks.
    QSet<int> used;
    for (const auto &belt : kBelts) {
        int lesson = 0;
        for (const char *const *needle = belt.needles; needle && *needle; ++needle) {
            if (lesson >= belt.limit)
                break;
            const int pick = pickBind(binds, used, QString::fromUtf8(*needle));
            if (pick < 0)
                continue;
            used.insert(pick);
            binds[pick].belt = QString::fromUtf8(belt.id);
            binds[pick].lesson = lesson++;
        }
    }
    return binds;
}

QString previewBindings()
{
    return QStringLiteral(
        "SUPER + SPACE                       → Omarchy menu\n"
        "SUPER + RETURN                      → Terminal\n"
        "SUPER SHIFT + RETURN                → Browser\n"
        "SUPER + W / SUPER + Q               → Close window\n"
        "SUPER + K                           → Keybindings\n"
        "SUPER SHIFT + F                     → File manager\n"
        "SUPER + ESCAPE                      → System menu\n");
}

int beltCount(const QList<Bind> &binds, const QString &belt)
{
    int count = 0;
    for (const auto &bind : binds)
        if (bind.belt == belt && bind.practiceable)
            ++count;
    return count;
}

QList<Bind> beltBinds(const QList<Bind> &binds, const QString &belt)
{
    QList<Bind> chosen;
    for (const auto &bind : binds)
        if (bind.belt == belt && bind.practiceable)
            chosen.append(bind);
    std::sort(chosen.begin(), chosen.end(), [](const Bind &a, const Bind &b) { return a.lesson < b.lesson; });
    return chosen;
}

QString teachLine(const QString &description)
{
    const auto lower = description.toLower();
    if (lower == "omarchy menu")
        return t("The launcher. Every app starts here.");
    if (lower == "terminal")
        return t("A shell, from whatever desk you are on.");
    if (lower == "browser")
        return t("The web, without hunting for an icon.");
    if (lower == "close window")
        return t("This window is done. The others stay.");
    if (lower == "keybindings")
        return t("The cheat sheet, for the bind you forgot.");
    if (lower == "file manager")
        return t("Your files, from anywhere.");
    if (lower == "system menu")
        return t("Lock, power, and the way out.");
    if (lower == "full screen")
        return t("This window takes the whole display.");
    if (lower == "full width")
        return t("Stretch it across. Keep the height.");
    if (lower == "toggle window floating/tiling")
        return t("Lift it out of the tiles, or drop it back in.");
    if (lower == "toggle window split")
        return t("Change how the tiles share the space.");
    if (lower.startsWith("pop window out"))
        return t("Float it, and keep it on top.");
    if (lower == "toggle transparency on a window")
        return t("See what is behind it.");
    if (lower == "next workspace")
        return t("The next desk.");
    if (lower == "previous workspace")
        return t("The desk beside this one.");
    if (lower == "former workspace")
        return t("The desk you were just on.");
    if (lower == "screenshot")
        return t("Grab the screen.");
    if (lower == "screenrecording")
        return t("Record the screen.");
    if (lower == "color picker")
        return t("Steal a color off the screen.");
    if (lower.startsWith("extract text"))
        return t("Read the words off the screen.");
    if (lower == "clipboard manager")
        return t("What you copied, still there.");
    if (lower == "lock system")
        return t("Step away. The session stays.");
    if (lower == "theme menu")
        return t("Change the look without leaving the keys.");
    if (lower == "toggle top bar")
        return t("The bar, when you want the pixels back.");
    if (lower == "toggle locking on idle")
        return t("Whether a quiet machine locks itself.");
    if (lower == "power menu")
        return t("Sleep, restart, or stop.");
    if (lower == "editor")
        return t("The editor, from anywhere.");
    if (lower == "universal copy")
        return t("Copy, even where the app ignores Ctrl+C.");
    if (lower == "universal paste")
        return t("Paste, even where the app ignores Ctrl+V.");
    if (lower == "universal cut")
        return t("Cut, even where the app ignores Ctrl+X.");
    if (lower == "emojis")
        return t("The emoji picker.");
    if (lower == "email")
        return t("Mail, from anywhere.");
    if (lower == "music")
        return t("Music, from anywhere.");
    if (lower == "calculator")
        return t("A calculator, from anywhere.");
    if (lower == "dismiss last notification")
        return t("Send the latest notification away.");
    if (lower == "dismiss all notifications")
        return t("Clear the stack.");
    if (lower == "toggle silencing notifications")
        return t("Quiet, until you want them again.");
    if (lower == "open notification history")
        return t("The ones you already dismissed.");
    if (lower == "toggle scratchpad")
        return t("A pocket that sits off the desks.");
    if (lower == "move window to scratchpad")
        return t("Stash this window in the pocket.");
    if (lower == "set reminder")
        return t("Ask the desktop to nudge you.");
    if (lower == "focus on left window")
        return t("Look left. The pointer stays.");
    if (lower == "focus on right window")
        return t("Look right. The pointer stays.");
    if (lower == "focus on above window")
        return t("Look up. The pointer stays.");
    if (lower == "focus on below window")
        return t("Look down. The pointer stays.");
    if (lower == "focus on next window")
        return t("The next window in line.");
    if (lower == "focus on previous window")
        return t("The window you just left.");
    if (lower == "swap window to the left")
        return t("Trade places with the window on the left.");
    if (lower == "swap window to the right")
        return t("Trade places with the window on the right.");

    static const QRegularExpression deskSwitch("^switch to workspace (\\d+)$");
    static const QRegularExpression deskMove("^move window to workspace (\\d+)$");
    if (const auto match = deskSwitch.match(lower); match.hasMatch())
        return t("Jump to desk %1. The windows stay where you left them.").arg(match.captured(1));
    if (const auto match = deskMove.match(lower); match.hasMatch())
        return t("Take this window to desk %1.").arg(match.captured(1));

    return t("Same keys every time. Let your hands learn them.");
}
