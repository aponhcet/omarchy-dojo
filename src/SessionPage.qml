import QtQuick
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: backend.sessionKicker + "  ·  " + backend.sessionTitle
                color: theme.accent
                font.pixelSize: 13
                font.letterSpacing: 1
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            Text {
                text: {
                    var _ = translator.language
                    return backend.sessionIndex.toLocaleString(Qt.locale(), "f", 0) + " / " + backend.sessionTotal.toLocaleString(Qt.locale(), "f", 0)
                }
                color: "#b7b7c0"
                font.pixelSize: 14
            }
            Text {
                visible: backend.lives >= 0
                text: {
                    var _ = translator.language
                    var marks = ""
                    for (var i = 0; i < 3; ++i)
                        marks += i < backend.lives ? "● " : "○ "
                    return marks
                }
                color: theme.accent
                font.pixelSize: 14
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 4
            radius: Math.min(2, theme.radius)
            color: "#2a2a30"
            Rectangle {
                width: parent.width * backend.sessionProgress
                height: parent.height
                radius: parent.radius
                color: theme.accent
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                anchors.centerIn: parent
                width: Math.min(760, parent.width)
                spacing: 16

                Text {
                    objectName: "actionLabel"
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: backend.sessionAction
                    color: "#f4f4f5"
                    font.pixelSize: 36
                    wrapMode: Text.WordWrap
                }

                Keycaps {
                    id: keys
                    objectName: "keyRow"
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    parts: backend.keyParts
                    ghost: !backend.showKeys
                }

                Text {
                    visible: backend.altKeyParts.length > 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("or")
                    color: "#8d8d96"
                    font.pixelSize: 13
                }

                Keycaps {
                    visible: backend.altKeyParts.length > 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    parts: backend.altKeyParts
                    ghost: false
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: backend.sessionWhen
                    color: "#b7b7c0"
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                }

                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: backend.coach
                    color: backend.coachTone === "miss" ? "#ff6b7a" : theme.accent
                    font.pixelSize: 18
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: backend.combo >= 2
                    text: {
                        var _ = translator.language
                        return "×" + backend.combo.toLocaleString(Qt.locale(), "f", 0)
                    }
                    color: theme.accent
                    font.pixelSize: 22
                }
            }
        }
    }
}
