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

Components.ChoiceList {

    id: choicelist

    property int buttonHeight: backgroundItem.sizeFromContents(100, 18).height
    property int buttonWidth: backgroundItem.sizeFromContents(100, 18).width

    property string hint

    height: buttonHeight
    width: buttonWidth
    topMargin: 4
    bottomMargin: 4

    background: QStyleItem {
        anchors.fill: parent
        elementType: "combobox"
        sunken: pressed
        raised: !pressed
        hover: containsMouse
        enabled: choicelist.enabled
        text: currentItemText
        focus: choicelist.focus
        hint: choicelist.hint
    }

    listItem: Item {
        id:item

        height: 22
        anchors.left: parent.left
        width: choicelist.width
        QStyleItem {
            anchors.fill: parent
            elementType: "comboboxitem"
            text: itemText
            selected: highlighted

        }
    }
    popupFrame: QStyleItem {
        property string popupLocation: backgroundItem.styleHint("comboboxpopup") ? "center" : "below"
        property int fw: backgroundItem.pixelMetric("menupanelwidth");
        anchors.leftMargin: backgroundItem.pixelMetric("menuhmargin") + fw
        anchors.rightMargin: backgroundItem.pixelMetric("menuhmargin") + fw
        anchors.topMargin: backgroundItem.pixelMetric("menuvmargin") + fw
        anchors.bottomMargin: backgroundItem.pixelMetric("menuvmargin") + fw
        elementType: "menu"

        effect: DropShadow {
            blurRadius: 18
            color: "#90000000"
            xOffset: 1
            yOffset: 1
        }
    }
}
