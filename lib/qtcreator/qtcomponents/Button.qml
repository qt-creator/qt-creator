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
import "custom" as Components

Components.Button {
    id:button

    // dm: this is wrong
    width: Math.max(sizehint.width, button.iconSource !== "" ? labelItem.contentsWidth + 8 : 0 )
    height: Math.max(22, sizehint.height)

    property variant sizehint: backgroundItem.sizeFromContents(80, 6)
    property bool defaultbutton
    property string hint

    background: QStyleItem {
        id: styleitem
        anchors.fill: parent
        elementType: "button"
        sunken: pressed || checked
        raised: !(pressed || checked)
        hover: containsMouse
        text: button.iconSource === "" ? button.text : ""
        focus: button.focus
        hint: button.hint

        // If no icon, let the style do the drawing
        activeControl: focus ? "default" : ""
        Connections{
            target: button
            onToolTipTriggered: styleitem.showTip()
        }
        function showTip(){
            showToolTip(tooltip);
        }
    }

    label: Item {
        // Used as a fallback since I can't pass the imageURL
        // directly to the style object
        visible: button.iconSource !== ""
        property int contentsWidth : row.width
        Row {
            id: row
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -1
            spacing: 4
            Image {
                source: iconSource
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.Stretch //mm Image should shrink if button is too small, depends on QTBUG-14957
            }
            Text {
                id:text
                color: textColor
                anchors.verticalCenter: parent.verticalCenter
                text: button.text
                horizontalAlignment: Text.Center
            }
        }
    }
    Keys.onSpacePressed:clicked()
}

