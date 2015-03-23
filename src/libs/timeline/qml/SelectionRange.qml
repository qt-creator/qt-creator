/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

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
        target: zoomer
        onWindowChanged: updateRange()
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
