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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0

Row {
    id: alignmentHorizontalButtons

    RoundedPanel {
        width: 14
        height: parent.height
        roundLeft: true
        ExtendedFunctionButton {
            anchors.verticalCenter: parent.verticalCenter
            backendValue: alignmentHorizontalButtons.backendValue
        }
    }

    property bool blueHighlight: false

    property variant backendValue: backendValues.horizontalAlignment;

    property variant value: backendValue.enumeration

    property bool baseStateFlag: isBaseState;

    onValueChanged: {
        if (value !== undefined) {
            if (value === "AlignLeft") {
                buttonRow.initalChecked = 0
                buttonRow.checkedIndex = 0
            } else if (value === "AlignHCenter") {
                buttonRow.initalChecked = 1
                buttonRow.checkedIndex = 1
            } else if (value === "AlignRight") {
                buttonRow.initalChecked = 2
                buttonRow.checkedIndex = 2
            }
        }
        evaluate()
    }

    property bool isInModel: backendValue.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: backendValue.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (baseStateFlag) {
            if (backendValue !== null && backendValue.isInModel)
                blueHighlight = true;
            else
                blueHighlight = false;
        } else {
            if (backendValue !== null && backendValue.isInSubState)
                blueHighlight = true;
            else
                blueHighlight = false;
        }
    }

    ButtonRow {
        id: buttonRow
        exclusive: true

        ButtonRowButton {
            roundLeftButton: false
            iconSource: blueHighlight ? "images/alignmentleft-h-icon.png" : "images/alignmentleft-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignLeft")
            }
        }
        ButtonRowButton {
            iconSource:  blueHighlight ? "images/alignmentcenterh-h-icon.png" : "images/alignmentcenterh-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignHCenter")
            }
        }
        ButtonRowButton {
            iconSource: blueHighlight ? "images/alignmentright-h-icon.png" : "images/alignmentright-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignRight")
            }
        }
    }
}
