// Event listeners.
document.addEventListener("DOMContentLoaded", statusColorize());

// Websockets.
var socket = io();
socket.on('connect', function() {
});
socket.on('statusupdate', function(data) {
    newstatus = "EMPTY";
    if (data.status == 1) {
        newstatus = "OCCUPIED";
    }
    var sensorrow = document.getElementById("sensor" + data.id);
    sensorrow.children[4].children[0].innerText = newstatus;
    sensorrow.children[3].innerText = data.updatedAt;
    statusColorize();
});

// Functions.
function statusColorize() {
    const emptyclass = "btn btn-success btn-sm";
    const occpdclass = "btn btn-danger btn-sm"
    var sensortable = document.getElementById("sensors");
    for (var i = 0, row; row = sensortable.rows[i]; i++) {
        if (row.children[4].children[0].innerText == "EMPTY") {
            row.children[4].children[0].className = emptyclass;

        }
        else {
            row.children[4].children[0].className = occpdclass;
        }
    }
}