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
    id: lineEdit

    function escapeString(string) {
        var str  = string;
        str = str.replace(/\\/g, "\\\\");
        str.replace(/\"/g, "\\\"");
        str = str.replace(/\t/g, "\\t");
        str = str.replace(/\r/g, "\\r");
        str = str.replace(/\n/g, '\\n');
        return str;
    }

    property variant backendValue
    property alias enabled: lineEdit.enabled
    property variant baseStateFlag
    property alias text: lineEditWidget.text
    property alias readOnly: lineEditWidget.readOnly
    property alias translation: trCheckbox.visible
    property alias inputMask: lineEditWidget.inputMask

    minimumHeight: 24;

    onBaseStateFlagChanged: {
        evaluate();
    }

    property variant isEnabled: lineEdit.enabled
    onIsEnabledChanged: {
        evaluate();
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
        if (!enabled) {
            lineEditWidget.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    lineEditWidget.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    lineEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    QLineEdit {
        y: 2
        id: lineEditWidget
        styleSheet: "QLineEdit { padding-left: 32; }"
        width: lineEdit.width
        height: lineEdit.height
        toolTip: backendValue.isBound ? backendValue.expression : ""

        property string valueFromBackend: (backendValue === undefined || backendValue.value === undefined) ? "" : backendValue.valueToString;

        onValueFromBackendChanged: {
            if (backendValue.value === undefined)
                return;
            text = backendValue.valueToString;
        }

        onEditingFinished: {
            if (backendValue.isTranslated) {
                backendValue.expression = "qsTr(\"" + escapeString(text) + "\")"
            } else {
                backendValue.value = text
            }
            evaluate();
        }

        onFocusChanged: {
            if (focus)
                backendValue.lock();
            else
                backendValue.unlock();
        }


    }
    ExtendedFunctionButton {
        backendValue: lineEdit.backendValue
        y: 6
        x: 0
        visible: lineEdit.enabled
    }
    QCheckBox {
        id: trCheckbox
        y: 2
        styleSheetFile: "checkbox_tr.css";
        toolTip: qsTr("Translate this string")
        x: lineEditWidget.width - 22
        height: lineEdit.height - 2;
        width: 24
        visible: false
        checked: backendValue.isTranslated
        onToggled: {
            if (trCheckbox.checked) {
                backendValue.expression = "qsTr(\"" + escapeString(lineEditWidget.text) + "\")"
            } else {
                backendValue.value = lineEditWidget.text
            }
            evaluate();
        }

        onVisibleChanged: {
            if (trCheckbox.visible) {
                trCheckbox.raise();
                lineEditWidget.styleSheet =  "QLineEdit { padding-left: 32; padding-right: 62;}"
            }
        }
    }
}
