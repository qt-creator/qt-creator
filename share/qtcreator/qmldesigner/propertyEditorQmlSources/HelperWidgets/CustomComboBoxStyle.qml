/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.2

ComboBoxStyle {
    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: "#eee"

    __editor: Item {

    }

    background: Item {
        implicitWidth: 120
        implicitHeight: 25

        RoundedPanel {
            anchors.fill: parent
            roundLeft: true
            roundRight: true
            visible: !control.pressed
        }

        RoundedPanel {
            gradient: Gradient {
                GradientStop {color: '#444' ; position: 0}
                GradientStop {color: '#333' ; position: 1}
            }
            anchors.fill: parent
            roundLeft: true
            roundRight: true
            visible: control.pressed
        }

        Rectangle {
            border.color: highlightColor
            anchors.fill: parent
            anchors.margins: -1
            color: "transparent"
            opacity: 0.3
            visible: control.activeFocus
        }

        Rectangle {
            color: "#333"
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
            source: "images/down-arrow.png"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 10
            opacity: control.enabled ? 0.7 : 0.5
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
            color: textColor
        }
    }

    __dropDownStyle: MenuStyle {
        __maxPopupHeight: 600
        __menuItemType: "comboboxitem"
        frame: Rectangle {
        }
    }
}
