import QtQuick
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 18

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: hero.implicitHeight + 36
            radius: theme.radius
            color: "#16161a"
            border.width: 1
            border.color: theme.accent

            Column {
                id: hero
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 20
                spacing: 8

                Text {
                    text: backend.heroKicker.toUpperCase()
                    color: theme.accent
                    font.pixelSize: 12
                    font.letterSpacing: 2
                }
                Text {
                    width: parent.width
                    text: backend.heroTitle
                    color: "#f4f4f5"
                    font.pixelSize: 32
                    wrapMode: Text.WordWrap
                }
                Text {
                    width: parent.width
                    text: backend.heroDetail
                    color: "#b7b7c0"
                    font.pixelSize: 16
                    wrapMode: Text.WordWrap
                    visible: text.length > 0
                }
                Text {
                    visible: backend.heroAction.length > 0
                    text: qsTr("Space  ·  %1").arg(backend.heroAction)
                    color: theme.accentForeground
                    font.pixelSize: 15
                    topPadding: 8
                    leftPadding: 12
                    rightPadding: 12
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

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: path.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: path
                width: parent.width
                spacing: 6

                Repeater {
                    model: backend.rowCount
                    delegate: Rectangle {
                        required property int index
                        readonly property var row: {
                            var _ = backend.generation
                            return backend.rowAt(index)
                        }
                        width: path.width
                        height: Math.max(58, label.implicitHeight + 18)
                        radius: theme.radius
                        color: row.selected ? "#221f14" : "transparent"
                        border.width: 1
                        border.color: row.selected ? theme.accent : "#2a2a30"
                        opacity: row.state === "locked" ? 0.45 : 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Text {
                                text: row.kicker || ""
                                color: theme.accent
                                font.pixelSize: 11
                                font.letterSpacing: 1
                                Layout.preferredWidth: 110
                            }
                            Column {
                                id: label
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    width: parent.width
                                    text: row.title || ""
                                    color: "#f4f4f5"
                                    font.pixelSize: 16
                                    elide: Text.ElideRight
                                }
                                Text {
                                    width: parent.width
                                    text: row.detail || ""
                                    color: "#9a9aa3"
                                    font.pixelSize: 13
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            preventStealing: true
                            onClicked: backend.choose(index)
                        }
                    }
                }
            }
        }
    }
}
