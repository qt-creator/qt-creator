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

import QtQuick 2.9
import welcome 1.0

GridView {
    id: root
    cellHeight: 180
    cellWidth: 285

    clip: true

    signal itemSelected(int index, variant item)

    delegate: HoverOverDesaturate {
        id: hoverOverDesaturate
        imageSource: typeof(thumbnail) === "undefined" ? "images/thumbnail_test.png" : thumbnail;
        labelText: displayName

        SequentialAnimation {
            id: animation
            running: hoverOverDesaturate.visible

            PropertyAction {
                target: hoverOverDesaturate
                property: "scale"
                value: 0.0
            }
            PauseAnimation {
                duration: model.index > 0 ? 100 * model.index : 0
            }
            NumberAnimation {
                target: hoverOverDesaturate
                property: "scale"
                from: 0.0
                to: 1.0
                duration: 200
                easing.type: Easing.InOutExpo
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.itemSelected(index, root.model.get(index))
        }
    }
}
