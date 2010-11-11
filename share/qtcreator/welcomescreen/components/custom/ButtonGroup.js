var self;
var checkHandlers = [];
var visibleButtons = [];
var nonVisibleButtons = [];
var direction;

function create(that, options) {
    self = that;
    direction = options.direction || Qt.Horizontal;
    self.childrenChanged.connect(rebuild);
//    self.widthChanged.connect(resizeChildren);
    build();
}

function isButton(item) {
    if (item && item.hasOwnProperty("__position"))
        return true;
    return false;
}

function hasChecked(item) {
    return (item && item.hasOwnProperty("checked"));
}

function destroy() {
    self.childrenChanged.disconnect(rebuild);
//    self.widthChanged.disconnect(resizeChildren);
    cleanup();
}

function build() {
    visibleButtons = [];
    nonVisibleButtons = [];

    for (var i = 0, item; (item = self.children[i]); i++) {
        if (!hasChecked(item))
            continue;

        item.visibleChanged.connect(rebuild); // Not optimal, but hardly a bottleneck in your app
        if (!item.visible) {
            nonVisibleButtons.push(item);
            continue;
        }
        visibleButtons.push(item);

        if (self.exclusive && item.hasOwnProperty("checkable"))
            item.checkable = true;

        if (self.exclusive) {
            item.checked = false;
            checkHandlers.push(checkExclusive(item));
            item.checkedChanged.connect(checkHandlers[checkHandlers.length - 1]);
        }
    }

    var nrButtons = visibleButtons.length;
    if (nrButtons == 0)
        return;

    if (self.checkedButton)
        self.checkedButton.checked = true;
    else if (self.exclusive) {
        self.checkedButton = visibleButtons[0];
        self.checkedButton.checked = true;
    }

    if (nrButtons == 1) {
        finishButton(visibleButtons[0], "only");
    } else {
        finishButton(visibleButtons[0], direction == Qt.Horizontal ? "leftmost" : "top");
        for (var i = 1; i < nrButtons - 1; i++)
            finishButton(visibleButtons[i], direction == Qt.Horizontal ? "h_middle": "v_middle");
        finishButton(visibleButtons[nrButtons - 1], direction == Qt.Horizontal ? "rightmost" : "bottom");
    }
}

function finishButton(button, position) {
    if (isButton(button)) {
        button.__position = position;
        if (direction == Qt.Vertical) {
            button.anchors.left = self.left     //mm How to make this not cause binding loops? see QTBUG-17162
            button.anchors.right = self.right
        }
    }
}

function cleanup() {
    visibleButtons.forEach(function(item, i) {
        if (checkHandlers[i])
            item.checkedChanged.disconnect(checkHandlers[i]);
        item.visibleChanged.disconnect(rebuild);
    });
    checkHandlers = [];

    nonVisibleButtons.forEach(function(item, i) {
        item.visibleChanged.disconnect(rebuild);
    });
}

function rebuild() {
    if (self == undefined)
        return;

    cleanup();
    build();
}

function resizeChildren() {
    if (direction != Qt.Horizontal)
        return;

    var extraPixels = self.width % visibleButtons;
    var buttonSize = (self.width - extraPixels) / visibleButtons;
    visibleButtons.forEach(function(item, i) {
        if (!item || !item.visible)
            return;
        item.width = buttonSize + (extraPixels > 0 ? 1 : 0);
        if (extraPixels > 0)
            extraPixels--;
    });
}

function checkExclusive(item) {
    var button = item;
    return function() {
        for (var i = 0, ref; (ref = visibleButtons[i]); i++) {
            if (ref.checked == (button === ref))
                continue;

            // Disconnect the signal to avoid recursive calls
            ref.checkedChanged.disconnect(checkHandlers[i]);
            ref.checked = !ref.checked;
            ref.checkedChanged.connect(checkHandlers[i]);
        }
        self.checkedButton = button;
    }
}
