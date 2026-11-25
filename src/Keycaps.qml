import QtQuick

// The bind, drawn as separate caps so Super and the letter read as a chord.
Flow {
    id: caps
    property var parts: []
    property bool ghost: false
    spacing: 8

    // A string list stored in a var does not populate a Repeater model. The
    // count does, and each cap reads its own entry.
    Repeater {
        model: caps.parts.length
        delegate: Row {
            required property int index
            spacing: 8

            Text {
                visible: index > 0
                width: visible ? implicitWidth : 0
                text: "+"
                color: "#8d8d96"
                font.pixelSize: 16
                height: 46
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                radius: Math.min(4, theme.radius)
                color: caps.ghost ? "transparent" : "#221f14"
                border.width: 1
                border.color: caps.ghost ? "#3c3c44" : theme.accent
                implicitWidth: Math.max(48, cap.implicitWidth + 22)
                implicitHeight: 46

                Text {
                    id: cap
                    objectName: "keycap"
                    anchors.centerIn: parent
                    text: String(caps.parts[index])
                    color: caps.ghost ? "#bdbdc6" : "#f4f4f5"
                    font.pixelSize: 16
                }
            }
        }
    }
}
