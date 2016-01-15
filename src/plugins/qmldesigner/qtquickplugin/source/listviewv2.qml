/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0

ListView {
    width: 110
    height: 160
    model:  ListModel {
        ListElement {
            name: "Grey"
            colorCode: "grey"
        }
        ListElement {
            name: "Red"
            colorCode: "red"
        }
        ListElement {
            name: "Blue"
            colorCode: "blue"
        }
        ListElement {
            name: "Green"
            colorCode: "green"
        }
    }

    delegate:  Item {
        width: 80
        height: 40
        x: 5
        Row {
            id: row1
            spacing: 10
            Rectangle { width: 40; height: 40; color: colorCode; }
            Text {
                text: name
                anchors.verticalCenter: parent.verticalCenter
                font.bold: true
            }
        }
    }
}
