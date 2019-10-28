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

import QtQuick 2.0
import QtQuick3D 1.0

Node {
    id: arrows

    property View3D view3D
    property bool highlightOnHover: false
    property Node targetNode: null
    readonly property bool isDragging: arrowX.isDragging || arrowY.isDragging || arrowZ.isDragging

    scale: Qt.vector3d(5, 5, 5)

    property alias arrowX: arrowX
    property alias arrowY: arrowY
    property alias arrowZ: arrowZ

    signal positionCommit()

    Arrow {
        id: arrowX
        objectName: "Arrow X"
        rotation: Qt.vector3d(0, -90, 0)
        targetNode: arrows.targetNode
        color: highlightOnHover && hovering ? Qt.lighter(Qt.rgba(1, 0, 0, 1))
                                            : Qt.rgba(1, 0, 0, 1)
        view3D: arrows.view3D

        onPositionCommit: arrows.positionCommit()
    }

    Arrow {
        id: arrowY
        objectName: "Arrow Y"
        rotation: Qt.vector3d(90, 0, 0)
        targetNode: arrows.targetNode
        color: highlightOnHover && hovering ? Qt.lighter(Qt.rgba(0, 0, 1, 1))
                                            : Qt.rgba(0, 0, 1, 1)
        view3D: arrows.view3D

        onPositionCommit: arrows.positionCommit()
    }

    Arrow {
        id: arrowZ
        objectName: "Arrow Z"
        rotation: Qt.vector3d(0, 180, 0)
        targetNode: arrows.targetNode
        color: highlightOnHover && hovering ? Qt.lighter(Qt.rgba(0, 0.6, 0, 1))
                                            : Qt.rgba(0, 0.6, 0, 1)
        view3D: arrows.view3D

        onPositionCommit: arrows.positionCommit()
    }
}
