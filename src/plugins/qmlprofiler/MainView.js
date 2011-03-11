.pragma library

var values = [ ];   //events
var ranges = [ ];
var frameFps = [ ];
var valuesdone = false;
var xmargin = 0;
var ymargin = 0;

var names = [ "Painting", "Compiling", "Creating", "Binding", "Handling Signal"]
//### need better way to manipulate color from QML. In the meantime, these need to be kept in sync.
var colors = [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ];
var origColors = [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ];
var xRayColors = [ "#6699CCB3", "#6699CCCC", "#6699B3CC", "#669999CC", "#66CC99B3", "#66CC99CC", "#66CCCC99", "#66CCB399" ];

function reset()
{
    values = [];
    ranges = [];
    frameFps = [];
    xmargin = 0;
    ymargin = 0;
    valuesdone = false;
}

function calcFps()
{
    if (values.length)
        frameFps = new Array(values.length - 1);
    for (var i = 0; i < values.length - 1; ++i) {
        var frameTime = (values[i + 1] - values[i]) / 1000000;
        frameFps[i] = 1000 / frameTime;
    }
}

//draw background of the graph
function drawGraph(canvas, ctxt, region)
{
    var grad = ctxt.createLinearGradient(0, 0, 0, canvas.canvasSize.height);
    grad.addColorStop(0,   '#fff');
    grad.addColorStop(1, '#ccc');
    ctxt.fillStyle = grad;

    ctxt.fillRect(0, 0, canvas.canvasSize.width + xmargin, canvas.canvasSize.height - ymargin);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt, region)
{
    if (values.length == 0 && ranges.length == 0)
        return;

    var width = canvas.canvasSize.width - xmargin;
    var height = canvas.height - ymargin;

    var sumValue = ranges[ranges.length - 1].start + ranges[ranges.length - 1].duration - ranges[0].start;
    var spacing = width / sumValue;

    //### only draw those in range
    for (var ii = 0; ii < ranges.length; ++ii) {

        var xx = (ranges[ii].start - ranges[0].start) * spacing + xmargin;
        if (xx > region.x + region.width)
            continue;

        var size = ranges[ii].duration * spacing;
        if (xx + size < region.x)
            continue;

        if (size < 0.5)
            size = 0.5;

        ctxt.fillStyle = "rgba(0,0,0,1)" //colors[ranges[ii].type];
        ctxt.fillRect(xx, ranges[ii].type*10, size, 10);
    }

    //draw fps overlay
    var heightScale = height / 60;
    ctxt.beginPath();
    ctxt.moveTo(0,0);
    for (var i = 1; i < values.length; ++i) {
        var xx = (values[i] - ranges[0].start) * spacing + xmargin;
        ctxt.lineTo(xx, height - frameFps[i-1]*heightScale)
    }
    ctxt.lineTo(width, 0);
    ctxt.closePath();
    var grad = ctxt.createLinearGradient(0, 0, 0, canvas.canvasSize.height);
    grad.addColorStop(0, "rgba(255,128,128,.5)");
    grad.addColorStop(1, "rgba(255,0,0,.5)");
    ctxt.fillStyle = grad;
    ctxt.fill();
}

function plot(canvas, ctxt, region)
{
    drawGraph(canvas, ctxt, region);
    drawData(canvas, ctxt, region);
}

function xScale(canvas)
{
    if (values.length === 0 && ranges.length === 0)
        return;

    var width = canvas.canvasSize.width - xmargin;

    var sumValue = ranges[ranges.length - 1].start + ranges[ranges.length - 1].duration - ranges[0].start;
    var spacing = sumValue / width;
    return spacing;
}
