#pragma once

#include <QString>

// While a round is listening, Hyprland's binds would swallow Super before
// the window ever saw it. The omarchy-dojo submap has one bind of its own,
// Super+Shift+Escape, so a stuck round can still be escaped by hand.
class Keygrab {
public:
    explicit Keygrab(QString hyprctl, bool enabled);
    ~Keygrab();

    bool enter(QString *error);
    void leave();
    bool asleep() const { return m_asleep; }

private:
    bool run(const QString &dispatcher, QString *error);

    QString m_hyprctl;
    bool m_enabled = false;
    bool m_asleep = false;
};
