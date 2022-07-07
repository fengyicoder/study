import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4


TextField {
    style: TextFieldStyle {
        textColor: "black";
        background: Rectangle {
            radius: 2;
            implicitHeight: 24;
            implicitWidth: 100;
            border.color: "#333";
            border.width: 1;
        }
    }

}
