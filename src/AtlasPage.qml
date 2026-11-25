import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        TextField {
            id: query
            objectName: "atlasQuery"
            Layout.fillWidth: true
            placeholderText: qsTr("Search binds")
            color: "#f4f4f5"
            font.pixelSize: 16
            Material.theme: Material.Dark
            Material.accent: theme.accent
            Material.roundedScale: theme.radius
            onTextEdited: backend.setAtlasQuery(text)
            onActiveFocusChanged: backend.typing = activeFocus
            Component.onCompleted: forceActiveFocus()

            Connections {
                target: backend
                function onUpdated() {
                    if (query.text !== backend.atlasQuery)
                        query.text = backend.atlasQuery
                }
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6

            Repeater {
                model: backend.atlasFilters
                delegate: Rectangle {
                    required property string modelData
                    radius: Math.min(4, theme.radius)
                    color: backend.atlasFilter === modelData ? theme.accent : "transparent"
                    border.width: 1
                    border.color: backend.atlasFilter === modelData ? theme.accent : "#3c3c44"
                    implicitWidth: chip.implicitWidth + 16
                    implicitHeight: 28

                    Text {
                        id: chip
                        anchors.centerIn: parent
                        text: backend.filterLabel(modelData)
                        color: backend.atlasFilter === modelData ? theme.accentForeground : "#d0d0d6"
                        font.pixelSize: 12
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.setFilter(modelData)
                    }
                }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: backend.atlasCount
            spacing: 4
            currentIndex: backend.atlasSelected

            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

            delegate: Rectangle {
                required property int index
                readonly property var row: {
                    var _ = backend.generation
                    return backend.atlasAt(index)
                }
                width: list.width
                height: 58
                radius: theme.radius
                color: row.selected ? "#221f14" : "transparent"
                border.width: 1
                border.color: row.selected ? theme.accent : "#2a2a30"
                opacity: row.practiceable ? 1 : 0.45

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10
                    Column {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            width: parent.width
                            text: row.description || ""
                            color: "#f4f4f5"
                            font.pixelSize: 15
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: (row.belt || "") + "  ·  " + (row.keys || "")
                            color: "#9a9aa3"
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                    Text {
                        text: row.dots || ""
                        color: theme.accent
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    preventStealing: true
                    onClicked: backend.chooseAtlas(index)
                }
            }

            Text {
                anchors.centerIn: parent
                visible: backend.atlasCount === 0
                text: qsTr("Nothing under that name.")
                color: "#b7b7c0"
                font.pixelSize: 16
            }
        }
    }
}
