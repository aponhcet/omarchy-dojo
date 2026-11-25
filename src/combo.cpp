#include "combo.h"

#include <QSet>

namespace {

const QStringList kModOrder = {"SUPER", "CTRL", "ALT", "SHIFT"};

QString modAlias(const QString &token)
{
    const auto upper = token.toUpper();
    if (upper == "SUPER" || upper == "MOD4" || upper == "META" || upper == "WIN")
        return "SUPER";
    if (upper == "CTRL" || upper == "CONTROL")
        return "CTRL";
    if (upper == "ALT" || upper == "MOD1")
        return "ALT";
    if (upper == "SHIFT")
        return "SHIFT";
    return {};
}

QStringList orderMods(QStringList mods)
{
    QStringList ordered;
    for (const auto &mod : kModOrder)
        if (mods.contains(mod))
            ordered.append(mod);
    return ordered;
}

QString normalizeKey(QString key)
{
    key = key.trimmed();
    if (key.isEmpty())
        return {};
    const auto upper = key.toUpper();
    if (upper == "ENTER" || upper == "RETURN" || upper == "KP_ENTER")
        return "RETURN";
    if (upper == "ESC" || upper == "ESCAPE")
        return "ESCAPE";
    if (upper == "~" || upper == "`" || upper == "GRAVE" || upper == "ASCIITILDE")
        return "GRAVE";
    if (upper == "-" || upper == "MINUS")
        return "MINUS";
    if (upper == "=" || upper == "EQUAL" || upper == "PLUS")
        return "EQUAL";
    if (upper == " " || upper == "SPACE")
        return "SPACE";
    return upper;
}

QString prettyMod(const QString &mod)
{
    if (mod == "SUPER")
        return "Super";
    if (mod == "CTRL")
        return "Ctrl";
    if (mod == "ALT")
        return "Alt";
    if (mod == "SHIFT")
        return "Shift";
    return mod;
}

QString prettyKey(const QString &key)
{
    if (key == "RETURN")
        return "Enter";
    if (key == "ESCAPE")
        return "Esc";
    if (key == "SPACE")
        return "Space";
    if (key == "TAB")
        return "Tab";
    if (key == "LEFT")
        return QStringLiteral("←");
    if (key == "RIGHT")
        return QStringLiteral("→");
    if (key == "UP")
        return QStringLiteral("↑");
    if (key == "DOWN")
        return QStringLiteral("↓");
    if (key == "MINUS")
        return QStringLiteral("−");
    if (key == "EQUAL")
        return "=";
    if (key == "PRINT")
        return "Print";
    if (key == "BACKSPACE")
        return "Backspace";
    if (key == "DELETE")
        return "Delete";
    if (key == "HOME")
        return "Home";
    if (key == "GRAVE")
        return "~";
    if (key == "COMMA")
        return ",";
    if (key == "PERIOD")
        return ".";
    if (key == "SLASH")
        return "/";
    if (key == "BRACKETLEFT")
        return "[";
    if (key == "BRACKETRIGHT")
        return "]";
    if (key.size() == 1)
        return key;
    return key;
}

bool sameKey(const QString &a, const QString &b)
{
    if (a == b)
        return true;
    const auto in = [](const QString &key, std::initializer_list<const char *> names) {
        for (const char *name : names)
            if (key == QLatin1String(name))
                return true;
        return false;
    };
    if (in(a, {"RETURN", "ENTER", "KP_ENTER"}) && in(b, {"RETURN", "ENTER", "KP_ENTER"}))
        return true;
    if (in(a, {"ESCAPE", "ESC"}) && in(b, {"ESCAPE", "ESC"}))
        return true;
    if (in(a, {"PRINT", "PRINTSCREEN", "SYSREQ"}) && in(b, {"PRINT", "PRINTSCREEN", "SYSREQ"}))
        return true;
    if (in(a, {"GRAVE", "ASCIITILDE"}) && in(b, {"GRAVE", "ASCIITILDE"}))
        return true;
    return false;
}

} // namespace

QString Combo::id() const
{
    auto bits = mods;
    bits.append(key);
    return bits.join('+');
}

QString Combo::label() const
{
    return parts().join(" + ");
}

QStringList Combo::parts() const
{
    QStringList out;
    for (const auto &mod : mods)
        out.append(prettyMod(mod));
    out.append(prettyKey(key));
    return out;
}

Combo parseCombo(const QString &raw)
{
    Combo combo;
    const auto text = raw.simplified();
    if (text.isEmpty())
        return combo;

    QStringList mods;
    QString key;
    if (text.contains('+')) {
        const auto chunks = text.split('+');
        for (int i = 0; i < chunks.size() - 1; ++i) {
            for (const auto &token : chunks.at(i).split(' ', Qt::SkipEmptyParts)) {
                const auto alias = modAlias(token);
                if (!alias.isEmpty() && !mods.contains(alias))
                    mods.append(alias);
            }
        }
        key = chunks.last().simplified();
    } else {
        key = text;
    }

    combo.mods = orderMods(mods);
    combo.key = normalizeKey(key);
    return combo;
}

QList<Combo> parseAlternatives(const QString &left)
{
    // "SUPER + W / SUPER + Q" is one action with two accepted binds.
    QList<Combo> out;
    for (const auto &part : left.split('/')) {
        const auto combo = parseCombo(part);
        if (combo.valid())
            out.append(combo);
    }
    return out;
}

Combo comboFromQt(int key, int modifiers)
{
    switch (key) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:
    case Qt::Key_AltGr:
    case Qt::Key_CapsLock:
    case Qt::Key_NumLock:
        return {};
    default:
        break;
    }

    QStringList mods;
    if (modifiers & Qt::MetaModifier)
        mods.append("SUPER");
    if (modifiers & Qt::ControlModifier)
        mods.append("CTRL");
    if (modifiers & Qt::AltModifier)
        mods.append("ALT");
    if (modifiers & Qt::ShiftModifier)
        mods.append("SHIFT");

    QString name;
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        name = "RETURN";
        break;
    case Qt::Key_Escape:
        name = "ESCAPE";
        break;
    case Qt::Key_Tab:
        name = "TAB";
        break;
    case Qt::Key_Backtab:
        name = "TAB";
        if (!mods.contains("SHIFT"))
            mods.append("SHIFT");
        break;
    case Qt::Key_Space:
        name = "SPACE";
        break;
    case Qt::Key_Print:
    case Qt::Key_SysReq:
        name = "PRINT";
        break;
    case Qt::Key_Backspace:
        name = "BACKSPACE";
        break;
    case Qt::Key_Delete:
        name = "DELETE";
        break;
    case Qt::Key_Left:
        name = "LEFT";
        break;
    case Qt::Key_Right:
        name = "RIGHT";
        break;
    case Qt::Key_Up:
        name = "UP";
        break;
    case Qt::Key_Down:
        name = "DOWN";
        break;
    case Qt::Key_Home:
        name = "HOME";
        break;
    case Qt::Key_Minus:
        name = "MINUS";
        break;
    case Qt::Key_Underscore:
        name = "MINUS";
        if (!mods.contains("SHIFT"))
            mods.append("SHIFT");
        break;
    case Qt::Key_Equal:
        name = "EQUAL";
        break;
    case Qt::Key_Plus:
        name = "EQUAL";
        if (!mods.contains("SHIFT"))
            mods.append("SHIFT");
        break;
    case Qt::Key_Comma:
        name = "COMMA";
        break;
    case Qt::Key_Period:
        name = "PERIOD";
        break;
    case Qt::Key_Slash:
        name = "SLASH";
        break;
    case Qt::Key_Question:
        name = "SLASH";
        if (!mods.contains("SHIFT"))
            mods.append("SHIFT");
        break;
    case Qt::Key_BracketLeft:
        name = "BRACKETLEFT";
        break;
    case Qt::Key_BracketRight:
        name = "BRACKETRIGHT";
        break;
    case Qt::Key_QuoteLeft:
        name = "GRAVE";
        break;
    case Qt::Key_AsciiTilde:
        name = "GRAVE";
        if (!mods.contains("SHIFT"))
            mods.append("SHIFT");
        break;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
            name = QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
        else if (key >= Qt::Key_0 && key <= Qt::Key_9)
            name = QString::number(key - Qt::Key_0);
        else if (key >= Qt::Key_A && key <= Qt::Key_Z)
            name = QChar(char('A' + (key - Qt::Key_A)));
        else
            return {};
    }

    Combo combo;
    combo.mods = orderMods(mods);
    combo.key = name;
    return combo;
}

bool combosMatch(const Combo &expected, const Combo &got)
{
    return expected.mods == got.mods && sameKey(expected.key, got.key);
}

bool isPracticeableKey(const QString &key)
{
    if (key.isEmpty() || key.contains("MOUSE") || key.startsWith("XF86"))
        return false;
    static const QSet<QString> modifiers = {
        "ALT_L", "ALT_R", "SUPER_L", "SUPER_R", "CTRL_L", "CTRL_R",
        "SHIFT_L", "SHIFT_R", "META_L", "META_R", "CAPSLOCK", "NUMLOCK", "ALTGR",
    };
    return !modifiers.contains(key);
}
