import QtQuick 2.0

Canvas {
    width: 530;
    height: 300;
    id: root;
    onPaint: {
        var ctx = getContext("2d");
        ctx.lineWidth = 2;
        ctx.strokeStyle = "red";
        ctx.font = "42px sans-serif";
        var gradient = ctx.createLinearGradient(0, 0, width, height);
        gradient.addColorStop(0.0, Qt.rgba(0, 1, 0, 1));
        gradient.addColorStop(1.0, Qt.rgba(0, 0, 1, 1));
        ctx.fillStyle = gradient;

        ctx.beginPath();
        ctx.text("Fill Text on Path", 10, 50);
        ctx.fill();
        ctx.fillText("Fill Text", 10, 100);
        ctx.beginPath();
        ctx.text("Stroke Text on Path", 10, 150);
        ctx.stroke();
        ctx.strokeText(" Stroke Text", 10, 200);
        ctx.beginPath();
        ctx.text("Stroke and Fill Text on Path", 10, 250);
        ctx.stroke();
        ctx.fill();
    }
}
