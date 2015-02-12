/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0

Row {
    id: alignmentVerticalButtons

    RoundedPanel {
        width: 14
        height: parent.height
        roundLeft: true

        ExtendedFunctionButton {
            anchors.verticalCenter: parent.verticalCenter
            backendValue: alignmentVerticalButtons.backendValue
        }
    }

    property bool blueHighlight: false

    property variant backendValue: backendValues.verticalAlignment;

    property variant value: backendValue.enumeration

    property bool baseStateFlag: isBaseState;

    onValueChanged: {
        if (value !== undefined) {
            if (value === "AlignTop") {
                buttonRow.initalChecked = 0
                buttonRow.checkedIndex = 0
            } else if (value === "AlignVCenter") {
                buttonRow.initalChecked = 1
                buttonRow.checkedIndex = 1
            } else if (value === "AlignBottom") {
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
            iconSource: blueHighlight ? "images/alignmenttop-h-icon.png" : "images/alignmenttop-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignTop")
            }
        }
        ButtonRowButton {
            iconSource:  blueHighlight ? "images/alignmentmiddle-h-icon.png" : "images/alignmentmiddle-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignVCenter")
            }
        }
        ButtonRowButton {
            iconSource: blueHighlight ? "images/alignmentbottom-h-icon.png" : "images/alignmentbottom-icon.png"
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignBottom")
            }
        }
    }
}
