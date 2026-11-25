import QtQuick

Item {
    Column {
        anchors.centerIn: parent
        width: Math.min(680, parent.width)
        spacing: 12

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: backend.resultKicker
            color: theme.accent
            font.pixelSize: 13
            font.letterSpacing: 2
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: {
                var _ = translator.language
                return backend.resultHits.toLocaleString(Qt.locale(), "f", 0) + " / " + backend.resultTotal.toLocaleString(Qt.locale(), "f", 0)
            }
            color: "#f4f4f5"
            font.pixelSize: 72
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: {
                var _ = translator.language
                var seconds = (backend.resultAvgMs / 1000).toLocaleString(Qt.locale(), "f", 2)
                return qsTr("%1 s average").arg(seconds)
            }
            color: "#b7b7c0"
            font.pixelSize: 16
            visible: backend.resultAvgMs > 0
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: {
                var _ = translator.language
                return "+" + backend.resultXp.toLocaleString(Qt.locale(), "f", 0) + " XP"
            }
            color: theme.accent
            font.pixelSize: 20
            visible: backend.resultXp > 0
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: backend.resultDetail
            color: "#f4f4f5"
            font.pixelSize: 18
            wrapMode: Text.WordWrap
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Space  ·  %1").arg(backend.resultAction)
            color: theme.accentForeground
            font.pixelSize: 15
            topPadding: 8
            leftPadding: 14
            rightPadding: 14
            bottomPadding: 8

            Rectangle {
                anchors.fill: parent
                z: -1
                radius: Math.min(4, theme.radius)
                color: theme.accent
            }
        }
    }
}
