// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

DirectionalDraggable {
    id: arrow
    source: "../meshes/arrow.mesh"

    property vector3d dragAxis
    property int startPivotValue

    signal pivotCommit()
    signal pivotChange()
    onTargetNodeChanged: visible = targetNode != multiSelectionNode

    onPressed: startPivotValue = targetNode.pivot.x * dragAxis.x +
                                 targetNode.pivot.y * dragAxis.y +
                                 targetNode.pivot.z * dragAxis.z;

    onDragged: (mouseArea, sceneRelativeDistance, relativeDistance)=> {
        if (dragAxis.x)
            targetNode.pivot.x = startPivotValue + relativeDistance *20
        else if (dragAxis.y)
            targetNode.pivot.y = startPivotValue + relativeDistance *20
        else if (dragAxis.z)
            targetNode.pivot.z = startPivotValue + relativeDistance *20
        pivotChange()
    }
    onReleased: pivotCommit();
 }
