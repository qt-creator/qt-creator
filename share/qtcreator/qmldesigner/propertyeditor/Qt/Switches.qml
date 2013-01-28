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
    styleSheetFile: "switch.css";
    property variant specialModeIcon;
    specialModeIcon: "images/standard.png";
    maximumWidth: 300;
    minimumWidth: 300;
    layout: QHBoxLayout {
        topMargin: 0;
        bottomMargin: 0;
        leftMargin: 0;
        rightMargin: 40;
        spacing: 0;

        QPushButton {
            checkable: true;
            checked: true;
            id: standardMode;
            toolTip: qsTr("Special properties");
            //iconFromFile: "images/rect-icon.png";
            text: backendValues === undefined || backendValues.className === undefined || backendValues.className == "empty" ? "empty" : backendValues.className.value
            onClicked: {
                extendedMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = true;
                extendedPane.visible = false;
                layoutPane.visible = false;
            }
        }

        QPushButton {
            id: layoutMode;
            checkable: true;
            checked: false;
            toolTip: qsTr("Layout");
            text: qsTr("Layout");
            onClicked: {
                extendedMode.checked = false;
                standardMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = false;
                layoutPane.visible = true;
            }
        }

        QPushButton {
            id: extendedMode;
            toolTip: qsTr("Advanced properties");
            checkable: true;
            checked: false;
            text: qsTr("Advanced")
            onClicked: {
                standardMode.checked = false;
                layoutMode.checked = false;
                checked = true;
                standardPane.visible = false;
                extendedPane.visible = true;
                layoutPane.visible = false;
            }
        }

    }
}
