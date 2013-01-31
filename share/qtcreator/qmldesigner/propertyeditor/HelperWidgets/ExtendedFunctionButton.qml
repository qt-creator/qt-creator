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

AnimatedToolButton {
    id: extendedFunctionButton

    property variant backendValue

    hoverIconFromFile: "images/submenu.png";

    function setIcon() {
        if (backendValue == null)
            extendedFunctionButton.iconFromFile = "images/placeholder.png"
        else if (backendValue.isBound ) {
            if (backendValue.isTranslated) { //translations are a special case
                extendedFunctionButton.iconFromFile = "images/placeholder.png"
            } else {
                extendedFunctionButton.iconFromFile = "images/expression.png"
            }
        } else {
            if (backendValue.complexNode != null && backendValue.complexNode.exists) {
                extendedFunctionButton.iconFromFile = "images/behaivour.png"
            } else {
                extendedFunctionButton.iconFromFile = "images/placeholder.png"
            }
        }
    }

    onBackendValueChanged: {
        setIcon();
    }
    property bool isBoundBackend: backendValue.isBound;
    property string backendExpression: backendValue.expression;

    onIsBoundBackendChanged: {
        setIcon();
    }

    onBackendExpressionChanged: {
        setIcon();
    }

    toolButtonStyle: "Qt::ToolButtonIconOnly"
    popupMode: "QToolButton::InstantPopup";
    property bool active: false;

    iconFromFile: "images/placeholder.png";
    width: 14;
    height: 14;
    focusPolicy: "Qt::NoFocus";

    styleSheet: "*::down-arrow, *::menu-indicator { image: none; width: 0; height: 0; }";

    onActiveChanged: {
        if (active) {
            setIcon();
            opacity = 1;
        } else {
            opacity = 0;
        }
    }


    actions:  [
    QAction {
        text: qsTr("Reset")
        visible: backendValue.isInSubState || backendValue.isInModel
        onTriggered: {
            transaction.start();
            backendValue.resetValue();
            backendValue.resetValue();
            transaction.end();
        }

    },
    QAction {
        text: qsTr("Set Expression");
        onTriggered: {
            expressionEdit.globalY = extendedFunctionButton.globalY - 10;
            expressionEdit.backendValue = extendedFunctionButton.backendValue

            if ((expressionEdit.y + expressionEdit.height + 20) > frame.height)
                expressionEdit.y = frame.height - expressionEdit.height - 20

            expressionEdit.show();
            expressionEdit.raise();
            expressionEdit.active = true;
        }
    }
    ]
}
