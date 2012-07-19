/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

import Qt 4.7
import Bauhaus 1.0

QWidget {
    id: expressionEditor;
    x: 16
    y: -400
    width: frame.width - 22
    height: 40
    property bool active: false
    property variant backendValue;
    
    property bool selectionFlag: selectionChanged
    
    onSelectionFlagChanged: {
        expressionEdit.active = false;    
    }

    onActiveChanged: {
        if (active) {
            textEdit.plainText = backendValue.expression
            textEdit.setFocus();
            opacity = 1;
            height = 60

        } else {
            opacity = 0;
            height = 40;
        }
    }
    opacity: 0
    onOpacityChanged: {
        if (opacity == 0)
            lower();
    }
    Behavior on opacity {
        NumberAnimation {
            duration: 100
        }
    }
    Behavior on height {
        NumberAnimation {
            duration: 100
        }
    }
    ExpressionEdit {
        id: textEdit;
        styleSheet: "QTextEdit {border-radius: 0px;}"
        documentTitle: qsTr("Expression")

        width: expressionEdit.width - 10
        height: expressionEdit.height - 10
        horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff"
        verticalScrollBarPolicy: "Qt::ScrollBarAlwaysOff"

        onFocusChanged: {
            if (!focus)
                expressionEdit.active = false;
        }
        onReturnPressed: {
            expressionEdit.backendValue.expression = textEdit.plainText;
            expressionEdit.active = false;
        }
    }
    QPushButton {
        focusPolicy: "Qt::NoFocus";
        y: expressionEdit.height - 22;
        x: expressionEdit.width - 59;
        styleSheetFile: "applybutton.css";
        width: 29
        height: 19
        onClicked: {
            expressionEdit.backendValue.expression = textEdit.plainText;
            expressionEdit.active = false;
        }        
    }
    QPushButton {
        focusPolicy: "Qt::NoFocus";
        y: expressionEdit.height - 22;
        x: expressionEdit.width - 30;
        styleSheetFile: "cancelbutton.css";
        width: 29
        height: 19
        onClicked: {
            expressionEdit.active = false;
        }
    }
}
