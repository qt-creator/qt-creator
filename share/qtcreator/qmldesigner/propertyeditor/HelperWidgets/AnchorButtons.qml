/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 1.0
import Bauhaus 1.0


QWidget {
    id: anchorButtons
    fixedHeight: 32

    function isBorderAnchored() {
        return anchorBackend.leftAnchored || anchorBackend.topAnchored || anchorBackend.rightAnchored || anchorBackend.bottomAnchored;
    }

    function fill() {
        anchorBackend.fill();
    }

    function breakLayout() {
        anchorBackend.resetLayout()
    }

    QPushButton {

        checkable: true
        fixedWidth: 31
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight
        styleSheetFile: "anchortop.css";
        toolTip: enabled ? qsTr("Set top anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.topAnchored;
        onReleased: {
            if (checked) {
                anchorBackend.verticalCentered = false;
                anchorBackend.topAnchored = true;
            } else {
                anchorBackend.topAnchored = false;
            }
        }

    }

    QPushButton {

        x: 31
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorbottom.css";
        toolTip: enabled ? qsTr("Set bottom anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.bottomAnchored;
        onReleased: {
            if (checked) {
                anchorBackend.verticalCentered = false;
                anchorBackend.bottomAnchored = true;
            } else {
                anchorBackend.bottomAnchored = false;
            }
        }

    }
    QPushButton {

        x: 61
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorleft.css";
        toolTip: enabled ? qsTr("Set left anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.leftAnchored;
        onReleased: {
            if (checked) {
                    anchorBackend.horizontalCentered = false;
                    anchorBackend.leftAnchored = true;
                } else {
                    anchorBackend.leftAnchored = false;
                }
        }
    }

    QPushButton {

        x: 91
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorright.css";
        toolTip: enabled ? qsTr("Set right anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.rightAnchored;
        onReleased: {
            if (checked) {
                    anchorBackend.horizontalCentered = false;
                    anchorBackend.rightAnchored = true;
                } else {
                    anchorBackend.rightAnchored = false;
                }
        }
    }

    QPushButton {
        x: 121
        checkable: true
        fixedWidth: 19
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorspacer.css";


    }

    QPushButton {
        x: 140
        fixedWidth: 30
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorfill.css";
        toolTip: enabled ? qsTr("Fill to parent") : qsTr("Setting anchors in states is not supported.")
        checkable: true
        checked: anchorBackend.isFilled;

        onReleased: {              
            if (checked) {
                anchorBackend.fill();
            } else {
                anchorBackend.resetLayout();
            }         
        }
    }

    QPushButton {
        x: 170
        checkable: true
        fixedWidth: 19
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

        styleSheetFile: "anchorspacer.css";

    }

    QPushButton {
        x: 189
        checkable: true
        fixedWidth: 30
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight

		styleSheetFile: "anchorvertical.css";
                toolTip: enabled ? qsTr("Set vertical anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.verticalCentered;
        onReleased: {
            if (checked) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
					anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                    anchorBackend.verticalCentered = true;
            } else {
                    anchorBackend.verticalCentered = false;
            }
        }
    }

    QPushButton {
        x: 219
        checkable: true
        fixedWidth: 31
		width: fixedWidth
        fixedHeight: 28
		height: fixedHeight
        
		styleSheetFile: "anchorhorizontal.css";
                toolTip: enabled ? qsTr("Set horizontal anchor") : qsTr("Setting anchors in states is not supported.")

        checked: anchorBackend.horizontalCentered;
        onReleased: {
            if (checked) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
                    anchorBackend.horizontalCentered = true;
            } else {
                    anchorBackend.horizontalCentered = false;
            }
        }
    }
}

