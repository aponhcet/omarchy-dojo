import QtQuick
import QtQuick.Controls.Material

Rectangle {
    objectName: "helpSheet"
    color: "#e6101014"
    visible: backend.helpOpen

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    MouseArea {
        anchors.fill: parent
        onClicked: backend.toggleHelp()
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(560, parent.width - 48)
        height: Math.min(sheet.implicitHeight + 48, parent.height - 48)
        radius: theme.radius
        color: "#16161a"
        border.width: 1
        border.color: theme.accent

        MouseArea { anchors.fill: parent }

        Column {
            id: sheet
            anchors.fill: parent
            anchors.margins: 24
            spacing: 14

            Text {
                text: qsTr("Shortcuts")
                color: theme.accent
                font.pixelSize: 13
                font.letterSpacing: 2
            }

            Repeater {
                model: {
                    var _ = translator.language
                    return [
                        qsTr("Space starts the selected round"),
                        qsTr("Arrows move"),
                        qsTr("A opens the atlas"),
                        qsTr("Enter drills the atlas row"),
                        qsTr("H reveals a hidden bind"),
                        qsTr("Tab skips a practice card"),
                        qsTr("Esc leaves, and asks first in a round"),
                        qsTr("Q quits, and asks first in a round"),
                        qsTr("? closes this list"),
                        qsTr("Super+Shift+Escape restores shortcuts if a round gets stuck")
                    ]
                }
                delegate: Text {
                    required property string modelData
                    width: sheet.width
                    text: modelData
                    color: "#f4f4f5"
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
