import QtQuick 2.0

Rectangle {
    id: root;
    Component.objectName: {
        var obj = new Object();
        console.log(obj.toString());
        console.log(obj.constructor());
        console.log(root.hasOwnProperty("width"));
        console.log(Item.isPrototypeOf(root));
        console.log(root.propertyIsEnumerable("children"));
        console.log(root.toString());
        console.log(root.valueOf());
    }
}
