#pragma once

#include "combo.h"

#include <QList>
#include <QString>

// One action on this machine. A line like "SUPER + W / SUPER + Q" keeps both
// combos, because either one is the right answer.
struct Bind {
    QList<Combo> combos;
    QString description;
    QString belt;
    int lesson = -1;
    bool practiceable = false;
    QString id;

    bool matches(const Combo &got) const;
    Combo primary() const { return combos.isEmpty() ? Combo{} : combos.first(); }
};

struct BeltDef {
    const char *id;
    const char *title;
    const char *blurb;
    int limit;
    const char *const *needles;
};

const BeltDef *beltDefs(int *count);
QList<Bind> loadBinds(const QString &text);
QString previewBindings();
QString teachLine(const QString &description);
int beltCount(const QList<Bind> &binds, const QString &belt);
QList<Bind> beltBinds(const QList<Bind> &binds, const QString &belt);
