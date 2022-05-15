import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.1

Window {
    visible: true;
    width: 600;
    height: 480;
    minimumWidth: 480;
    minimumHeight: 380;
    BusyIndicator {
        id: busy;
        running: false;
        anchors.centerIn: parent;
        z: 2;
    }

    Text {
        id: stateLable;
        visible: false;
        anchors.centerIn: parent;
        z: 3;
    }

    Image {
        id: imageViewer;
        asynchronous: true;
        cache: false;
        anchors.fill: parent;
        fillMode: Image.PreserveAspectFit;
        onStatusChanged: {
            if (imageViewer.status == Image.Loading) {
                busy.running = true;
                stateLable.visible = false;
            }
            else if(imageViewer.status == Image.Ready) {
                busy.running = false;
            }
            else if(imageViewer.status == Image.Error) {
                busy.running = false;
                stateLable.visible = true;
                stateLable.text = "ERROR";
            }
        }
    }

    Button {
        id: openFile;
        text: "Open";
        anchors.left: parent.left;
        anchors.leftMargin: 8;
        anchors.bottom: parent.bottom;
        anchors.bottomMargin: 8;
        style: ButtonStyle {
            background: Rectangle {
                implicitHeight: 25;
                implicitWidth: 70;
                color: control.hovered ? "#c0c0c0" : "#a0a0a0";
                border.width: control.pressed ? 2 : 1;
                border.color: (control.hovered || control.pressed) ? "green" : "#888888";
            }
        }
        onClicked: fileDialog.open();
        z: 4;
    }

    Text {
        id: imagePath;
        anchors.left: openFile.right;
        anchors.leftMargin: 8;
        anchors.verticalCenter: openFile.verticalCenter;
        font.pixelSize: 18;
    }

    FileDialog {
        id: fileDialog;
        title: "Please choose a file";
        nameFilters: ["Image Files (*.jpg *.png *.gif)"];
        onAccepted: {
            imageViewer.source = fileDialog.fileUrl;
            var imageFile = new String(fileDialog.fileUrl);
            imagePath.text = imageFile.slice(8);
        }
    }
}
