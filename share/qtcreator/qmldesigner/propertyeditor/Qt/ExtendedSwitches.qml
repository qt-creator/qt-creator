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

QFrame {
    visible: false;
    focusPolicy: "Qt::NoFocus"
    id: extendedSwitches;
    property bool active: false;
    property variant backendValue;
    styleSheetFile: "switch.css";
    property variant specialModeIcon;
    specialModeIcon: "images/standard.png";

    opacity: 0;

    Behavior on opacity {
        NumberAnimation {
            easing.type: "InSine"
            duration: 200
        }
    }

    onActiveChanged: {
        if (!nameLineEdit.focus)
            if (active) {
                opacity = 1;
            } else {
                opacity = 0;
            }
    }

    layout: QHBoxLayout {
        topMargin: 6;
        bottomMargin: 4;
        leftMargin: 10;
        rightMargin: 8;
        spacing: 4;

        QPushButton {
            focusPolicy: "Qt::NoFocus"
            onReleased:  backendValue.resetValue();
            iconFromFile: "images/reset-button.png";
            toolTip: "reset property";
        }

        QLineEdit {
            id: nameLineEdit;
            readOnly: false;
            //focusPolicy: "Qt::NoFocus"
            text: backendValue == null ? "" : backendValue.expression;
            onEditingFinished: {
                if (backendValue != null)
                    backendValue.expression = text;
            }
            onFocusChanged: {
                extendedSwitches.active = focus;
            }
        }
     }
}
