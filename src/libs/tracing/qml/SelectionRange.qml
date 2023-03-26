// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

RangeMover {
    id: selectionRange
    property QtObject zoomer;

    readonly property int creationInactive: 0
    readonly property int creationFirstLimit: 1
    readonly property int creationSecondLimit: 2
    readonly property int creationFinished: 3

    property bool ready: visible && creationState === creationFinished

    property double viewTimePerPixel: 1
    property double creationReference : 0
    property int creationState : creationInactive

    function reset() {
        rangeRight = rangeLeft + 1;
        creationState = creationInactive;
        creationReference = 0;
    }

    function updateZoomer() {
        zoomer.setSelection(rangeLeft * viewTimePerPixel + zoomer.windowStart,
                            rangeRight * viewTimePerPixel + zoomer.windowStart)
    }

    function updateRange() {
        var left = (zoomer.selectionStart - zoomer.windowStart) / viewTimePerPixel;
        var right = (zoomer.selectionEnd - zoomer.windowStart) / viewTimePerPixel;
        if (left < rangeLeft) {
            rangeLeft = left;
            rangeRight = right;
        } else {
            rangeRight = right;
            rangeLeft = left;
        }
    }

    onRangeWidthChanged: updateZoomer()
    onRangeLeftChanged: updateZoomer()

    Connections {
        target: selectionRange.zoomer
        function onWindowChanged() { selectionRange.updateRange(); }
    }

    function setPos(pos) {
        if (pos < 0)
            pos = 0;
        else if (pos > width)
            pos = width;

        switch (creationState) {
        case creationFirstLimit:
            creationReference = pos;
            rangeLeft = pos;
            rangeRight = pos + 1;
            break;
        case creationSecondLimit:
            if (pos > creationReference) {
                rangeLeft = creationReference;
                rangeRight = pos;
            } else if (pos < creationReference) {
                rangeLeft = pos;
                rangeRight = creationReference;
            }
            break;
        }
    }
}
