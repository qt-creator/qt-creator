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

QWidget { //This is a special doubleSpinBox that does color coding for states

    id: doubleSpinBox;

    property variant backendValue;
    property variant baseStateFlag;
    property alias singleStep: box.singleStep
    property alias minimum: box.minimum
    property alias maximum: box.maximum
    property alias spacing: layoutH.spacing
    property alias text: label.text
    property bool alignRight: true
    property bool enabled: true

    minimumHeight: 22;

    onBaseStateFlagChanged: {
        evaluate();
    }

    onBackendValueChanged: {
        evaluate();
    }

    onEnabledChanged: {
        evaluate();
    }

    function evaluate() {
        if (backendValue === undefined)
            return;
        if (!enabled) {
            box.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue.isInModel)
                    box.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue.isInSubState)
                    box.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    box.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    property bool isInModel: backendValue.isInModel;

    onIsInModelChanged: {
        evaluate();
    }

    property bool isInSubState: backendValue.isInSubState;

    onIsInSubStateChanged: {
        evaluate();
    }

    layout: HorizontalLayout {
        id: layoutH;

        QLabel {
            id: label;
            font.bold: true;
            alignment: doubleSpinBox.alignRight  ? "Qt::AlignRight | Qt::AlignVCenter" : "Qt::AlignLeft | Qt::AlignVCenter";
            fixedWidth: frame.labelWidth
            visible: doubleSpinBox.text != "";
			toolTip: text
        }

        QDoubleSpinBox {
            id: box;
            decimals: 1;
            keyboardTracking: false;
            enabled: !backendValue.isBound && doubleSpinBox.enabled;
            minimum: -1000
            maximum: 1000

            property bool readingFromBackend: false;
            property real valueFromBackend: doubleSpinBox.backendValue.value;

            onValueFromBackendChanged: {
                readingFromBackend = true;
                value = valueFromBackend
                readingFromBackend = false;
            }

            onValueChanged: {
                doubleSpinBox.backendValue.value = value;
            }

            onMouseOverChanged: {

            }

            onFocusChanged: {
                if (focus)
                transaction.start();
                else
                transaction.end();
            }
        }
    }

    ExtendedFunctionButton {
        backendValue: doubleSpinBox.backendValue;
        y: box.y + 4
        x: box.x + 2
        visible: doubleSpinBox.enabled
    }
}
