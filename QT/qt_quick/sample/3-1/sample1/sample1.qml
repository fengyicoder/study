import QtQuick 2.0

Rectangle {
    Component.onCompleted: {
        var name = "Jon";
        console.log(typeof name);
        console.log(typeof 60);
    }
}
