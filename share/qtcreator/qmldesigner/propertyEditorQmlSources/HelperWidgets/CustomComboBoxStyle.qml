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

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.2

ComboBoxStyle {
    property color textColor: creatorTheme.PanelTextColorLight
    __editor: Item {

    }

    background: Item {
        implicitWidth: 120
        implicitHeight: 25

        Rectangle {
            anchors.fill: parent
            visible: !control.pressed
            color: creatorTheme.FancyToolButtonSelectedColor
            border.color: creatorTheme.QmlDesignerBackgroundColorDarker
            border.width: 1
        }

        Rectangle {
            color: creatorTheme.FancyToolButtonHoverColor
            anchors.fill: parent
            visible: control.pressed
            border.color: creatorTheme.QmlDesignerBackgroundColorDarker
            border.width: 1
        }

        Rectangle {
            color: creatorTheme.IconsBaseColor
            width: 1
            anchors.right: imageItem.left
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            anchors.rightMargin: 6
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }

        Image {
            id: imageItem
            width: 8
            height: 4
            source: "image://icons/down-arrow"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            opacity: control.enabled ? 1 : 0.5
        }
    }

    label: Item {
        implicitWidth: textitem.implicitWidth + 20
        Text {
            id: textitem
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: control.currentText
            renderType: Text.NativeRendering
            color: creatorTheme.PanelTextColorLight
        }
    }

    __dropDownStyle: MenuStyle {
        __maxPopupHeight: 600
        __menuItemType: "comboboxitem"
        frame: Rectangle {
        }
    }
}
