import QtQuick 2.9
import QtQuick.Window 2.2

Rectangle {
    visible: true
    width: 640
    height: 480
//    title: qsTr("Hello World")
    MouseArea {
        anchors.fill: parent;
        onClicked: {
            Qt.quit();
        }
    }
    Text {
        text: qsTr("Hello Qt Quick App");
        anchors.centerIn: parent;
    }
}
