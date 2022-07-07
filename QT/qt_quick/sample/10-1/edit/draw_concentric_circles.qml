import QtQuick 2.0

Canvas {
    width: 300;
    height: 300;
    contextType: "2d";
    onPaint: {
        context.lineWidth = 2;
        context.strokeStyle = "blue";
        context.fillStyle = "red";
        context.save();
        context.translate(width/2, height/2);
        context.beginPath();
        context.arc(0, 0, 30, 0, Math.PI*2);
        context.arc(0, 0, 50, 0, Math.PI*2);
        context.arc(0, 0, 70, 0, Math.PI*2);
        context.arc(0, 0, 90, 0, Math.PI*2);
        context.stroke();
        context.restore();
        context.save();
        context.translate(width/2, 30);
        context.font = "26px serif";
        context.textAlign = "center";
        context.fillText("concentric circles", 0, 0);
        context.restore();
    }
}
