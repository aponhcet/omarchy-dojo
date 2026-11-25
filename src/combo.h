#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <Qt>

// A Hyprland combo, normalized so "SUPER SHIFT + return" and a Qt key event
// can be compared without the display text getting in the way.
struct Combo {
    QStringList mods;
    QString key;

    QString id() const;
    QString label() const;
    QStringList parts() const;
    bool valid() const { return !key.isEmpty(); }
};

Combo parseCombo(const QString &raw);
QList<Combo> parseAlternatives(const QString &left);
Combo comboFromQt(int key, int modifiers);
bool combosMatch(const Combo &expected, const Combo &got);
bool isPracticeableKey(const QString &key);
