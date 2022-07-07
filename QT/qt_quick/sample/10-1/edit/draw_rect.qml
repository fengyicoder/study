import QtQuick 2.0

Canvas {
    width: 400;
    height: 240;
    onPaint: {
        var ctx = getContext("2d");
        ctx.lineWidth = 2;
        ctx.strokeStyle = "red";
        ctx.fillStyle = "blue";
        ctx.beginPath();
        ctx.rect(100, 80, 120, 80);
        ctx.fill();
        ctx.stroke();
    }

}
