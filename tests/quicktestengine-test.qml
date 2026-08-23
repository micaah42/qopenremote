import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    id: _window
    objectName: "window"
    visible: true
    width: 400
    height: 400

    Item {
        id: _target
        objectName: "target"
        property int answer: 42
    }

    Column {
        id: _column
        objectName: "column"
        anchors.fill: parent
        spacing: 4

        Repeater {
            id: _repeater
            objectName: "repeater"
            model: 3

            Item {
                id: _repeaterItem
                objectName: "repeaterItem" + index
                property int itemIndex: index

                Item {
                    id: _nestedInRepeaterItem
                    objectName: "nestedInRepeaterItem" + index
                    property string label: "nested " + index
                }
            }
        }

        ListView {
            id: _listView
            objectName: "listView"
            width: parent.width
            height: 100
            model: ["first", "second", "third"]

            delegate: Item {
                id: _listViewDelegate
                objectName: "listViewDelegate" + index
                width: _listView.width
                height: 20
                property string value: modelData

                Text {
                    id: _listViewDelegateText
                    objectName: "listViewDelegateText" + index
                    text: modelData
                }
            }
        }

        Container {
            id: _container
            objectName: "container"
            width: parent.width
            height: 100

            contentItem: Item {
                id: _containerContent
                objectName: "containerContent"

                Item {
                    id: _deeplyNestedItem
                    objectName: "deeplyNestedItem"
                    property int depth: 3
                }
            }
        }
    }

    Item {
        id: _clickTarget
        anchors.right: parent.right
        objectName: "clickTarget"
        width: 50
        height: 50
        property int clickCount: 0

        Button {
            id: _mouseArea
            anchors.fill: parent
            onClicked: _clickTarget.clickCount++
        }
    }
}
