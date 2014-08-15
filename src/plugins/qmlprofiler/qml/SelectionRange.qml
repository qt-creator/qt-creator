/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

RangeMover {
    id: selectionRange

    property bool ready: visible && creationState === 3

    property string startTimeString: detailedPrintTime(startTime)
    property string endTimeString: detailedPrintTime(startTime+duration)
    property string durationString: detailedPrintTime(duration)

    property double startTime: getLeft() * viewTimePerPixel + zoomControl.windowStart()
    property double duration: Math.max(getWidth() * viewTimePerPixel, 500)
    property double viewTimePerPixel: 1
    property double creationReference : 0
    property int creationState : 0

    Connections {
        target: zoomControl
        onRangeChanged: {
            var oldTimePerPixel = selectionRange.viewTimePerPixel;
            selectionRange.viewTimePerPixel = Math.abs(zoomControl.endTime() - zoomControl.startTime()) / view.intWidth;
            if (creationState === 3 && oldTimePerPixel != selectionRange.viewTimePerPixel) {
                var newWidth = getWidth() * oldTimePerPixel / viewTimePerPixel;
                setLeft(getLeft() * oldTimePerPixel / viewTimePerPixel);
                setRight(getLeft() + newWidth);
            }
        }
    }

    onRangeDoubleClicked: {
        zoomControl.setRange(startTime, startTime + duration);
        root.selectionRangeMode = false;
    }

    function reset() {
        setRight(getLeft() + 1);
        creationState = 0;
        creationReference = 0;
    }

    function setPos(pos) {
        if (pos < 0)
            pos = 0;
        else if (pos > width)
            pos = width;

        switch (creationState) {
        case 1:
            creationReference = pos;
            setLeft(pos);
            setRight(pos + 1);
            break;
        case 2:
            if (pos > creationReference) {
                setLeft(creationReference);
                setRight(pos);
            } else if (pos < creationReference) {
                setLeft(pos);
                setRight(creationReference);
            }
            break;
        }
    }


    function detailedPrintTime( t )
    {
        if (t <= 0) return "0";
        if (t<1000) return t+" ns";
        t = Math.floor(t/1000);
        if (t<1000) return t+" μs";
        if (t<1e6) return (t/1000) + " ms";
        return (t/1e6) + " s";
    }

    // creation control
    function releasedOnCreation() {
        if (selectionRange.creationState === 2) {
            flick.stayInteractive = true;
            selectionRange.creationState = 3;
            selectionRangeControl.enabled = false;
        }
    }

    function pressedOnCreation() {
        if (selectionRange.creationState === 1) {
            flick.stayInteractive = false;
            selectionRange.setPos(selectionRangeControl.mouseX + flick.contentX);
            selectionRange.creationState = 2;
        }
    }

    function movedOnCreation() {
        if (selectionRange.creationState === 0) {
            selectionRange.creationState = 1;
        }

        if (!selectionRangeControl.pressed && selectionRange.creationState==3)
            return;

        selectionRange.setPos(selectionRangeControl.mouseX + flick.contentX);
    }
}
