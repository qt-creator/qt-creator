/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick 2.0
import QtQuick.Controls 2.0

Rectangle {
    property bool selected: false
    property string tooltip
    property string shortcut
    property string currentShortcut
    property string tool
    property string buttonGroup
    property bool togglable: true
    property string sceneId

    property int _buttonGroupIndex: -1
    property var _buttonGroupArray: []

    id: root
    width: 16
    height: 16
    color: root.selected ? "#aa000000" : (mouseArea.containsMouse ? "#44000000" : "#00000000")
    radius: 3

    Component.onCompleted: {
        var group = parent.buttonGroups[buttonGroup];
        if (group) {
            _buttonGroupArray = group;
            for (var i = 0; i < _buttonGroupArray.length; ++i) {
                if (_buttonGroupArray[i] === this) {
                    _buttonGroupIndex = i;
                    break;
                }
            }
        }
    }

    ToolTip {
        text: root.tooltip + " (" + root.shortcut + ")"
        visible: mouseArea.containsMouse
        delay: 1000
    }

    Shortcut {
        sequence: root.currentShortcut
        onActivated: mouseArea.onClicked(null)
    }

    MouseArea {
        id: mouseArea
        cursorShape: "PointingHandCursor"
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (!root.selected) {
                for (var i = 0; i < root._buttonGroupArray.length; ++i)
                    root._buttonGroupArray[i].selected = false;

                root.selected = true;
                if (_buttonGroupIndex >= 0)
                    _generalHelper.storeToolState(sceneId, root.buttonGroup, root._buttonGroupIndex)

                if (!root.togglable) {
                    // Deselect button after a short while (selection acts as simple click indicator)
                    _generalHelper.delayedPropertySet(root, 200, "selected", false);
                }
            }
        }
    }
}


