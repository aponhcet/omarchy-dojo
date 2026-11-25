import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1080
    height: 740
    minimumWidth: 860
    minimumHeight: 600
    visible: true
    title: qsTr("Omarchy Dojo")
    color: "#0e0e10"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        enabled: !backend.typing
        onActivated: backend.toggleHelp()
    }
    Shortcut {
        sequence: "Q"
        context: Qt.ApplicationShortcut
        enabled: !backend.typing
        onActivated: backend.askQuit()
    }
    Shortcut {
        sequence: "Space"
        context: Qt.ApplicationShortcut
        enabled: backend.page !== "atlas" && !backend.typing
        onActivated: backend.primaryAction()
    }
    Shortcut {
        sequence: "Return"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "atlas" || !backend.typing
        onActivated: backend.primaryAction()
    }
    Shortcut {
        sequence: "Enter"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "atlas" || !backend.typing
        onActivated: backend.primaryAction()
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: backend.back()
    }
    Shortcut {
        sequence: "Up"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "home" || backend.page === "atlas"
        onActivated: backend.move(-1)
    }
    Shortcut {
        sequence: "Down"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "home" || backend.page === "atlas"
        onActivated: backend.move(1)
    }
    Shortcut {
        sequence: "Left"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "atlas" && !backend.typing
        onActivated: backend.cycleFilter(-1)
    }
    Shortcut {
        sequence: "Right"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "atlas" && !backend.typing
        onActivated: backend.cycleFilter(1)
    }
    Shortcut {
        sequence: "A"
        context: Qt.ApplicationShortcut
        enabled: backend.page === "home" && !backend.typing
        onActivated: backend.showAtlas()
    }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 28
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                Column {
                    spacing: 2
                    Text {
                        text: qsTr("DOJO")
                        color: theme.accent
                        font.pixelSize: 14
                        font.letterSpacing: 4
                    }
                    Text {
                        text: backend.streakLine
                        color: "#9a9aa3"
                        font.pixelSize: 12
                        width: 360
                        elide: Text.ElideRight
                    }
                }
                Item { Layout.fillWidth: true }
                Column {
                    spacing: 2
                    Layout.alignment: Qt.AlignRight
                    Text {
                        anchors.right: parent.right
                        text: backend.rankName
                        color: "#f4f4f5"
                        font.pixelSize: 16
                    }
                    Text {
                        anchors.right: parent.right
                        text: {
                            var _ = translator.language
                            if (backend.rankTop)
                                return backend.rankTagline
                            return backend.xp.toLocaleString(Qt.locale(), "f", 0) + " / " + backend.xpNext.toLocaleString(Qt.locale(), "f", 0) + " XP"
                        }
                        color: "#9a9aa3"
                        font.pixelSize: 12
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 4
                radius: Math.min(2, theme.radius)
                color: "#2a2a30"
                Rectangle {
                    width: parent.width * backend.rankFraction
                    height: parent.height
                    radius: parent.radius
                    color: theme.accent
                }
            }

            Text {
                visible: backend.status.length > 0 && backend.page === "home"
                Layout.fillWidth: true
                text: backend.status
                color: "#ffb4bc"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: backend.page === "session" ? "SessionPage.qml"
                      : backend.page === "result" ? "ResultPage.qml"
                      : backend.page === "atlas" ? "AtlasPage.qml"
                      : "HomePage.qml"
            }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: "#8d8d96"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                text: {
                    var _ = translator.language
                    if (backend.helpOpen)
                        return qsTr("Esc closes help")
                    if (backend.page === "session")
                        return backend.sessionHint
                    if (backend.page === "result")
                        return qsTr("Space continues    Esc home    Q quits")
                    if (backend.page === "atlas")
                        return qsTr("Type to search    Enter drills    Left and right filter    Esc home")
                    return qsTr("Space starts    Arrows move    A atlas    Q quits    ? help")
                }
            }
        }

        HelpOverlay { anchors.fill: parent }
    }
}
