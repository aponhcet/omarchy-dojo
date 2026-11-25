#include "keygrab.h"

#include <QProcess>

Keygrab::Keygrab(QString hyprctl, bool enabled)
    : m_hyprctl(std::move(hyprctl)), m_enabled(enabled)
{
}

Keygrab::~Keygrab()
{
    leave();
}

bool Keygrab::run(const QString &dispatcher, QString *error)
{
    if (m_hyprctl.isEmpty()) {
        if (error)
            *error = QStringLiteral("missing");
        return false;
    }
    QProcess process;
    process.start(m_hyprctl, {"dispatch", dispatcher});
    if (!process.waitForFinished(1500)) {
        process.kill();
        process.waitForFinished(200);
        if (error)
            *error = QStringLiteral("timeout");
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error)
            *error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}

bool Keygrab::enter(QString *error)
{
    if (!m_enabled)
        return true;
    if (m_asleep)
        return true;
    if (!run(QStringLiteral("hl.dsp.submap(\"omarchy-dojo\")"), error))
        return false;
    m_asleep = true;
    return true;
}

void Keygrab::leave()
{
    if (!m_asleep)
        return;
    run(QStringLiteral("hl.dsp.submap(\"reset\")"), nullptr);
    m_asleep = false;
}
