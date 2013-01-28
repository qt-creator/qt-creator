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
