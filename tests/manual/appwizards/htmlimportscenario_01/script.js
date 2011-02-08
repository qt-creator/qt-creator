$(document).ready(function () {
    $("div").mousedown(function () {
        $(this).hide(400);
    });
    $("#quit").click(function () {
        Qt.quit();
    });
});
