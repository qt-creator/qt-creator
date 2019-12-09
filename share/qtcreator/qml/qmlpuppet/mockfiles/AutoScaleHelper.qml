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
import MouseArea3D 1.0

Node {
    id: overlayNode

    property View3D view3D
    property Node target: parent
    property bool autoScale: true
    property Camera camera: view3D.camera

    // Read-only
    property real relativeScale: 1

    onSceneTransformChanged: updateScale()
    onAutoScaleChanged: updateScale()
    // Trigger delayed update on camera change to ensure camera values are correct
    onCameraChanged: _generalHelper.requestOverlayUpdate();

    Connections {
        target: camera
        onSceneTransformChanged: updateScale()
    }

    Connections {
        target: _generalHelper
        onOverlayUpdateNeeded: updateScale()
    }

    function getScale(baseScale)
    {
        return Qt.vector3d(baseScale.x * relativeScale, baseScale.y * relativeScale,
                           baseScale.z * relativeScale);
    }

    function updateScale()
    {
        if (!autoScale) {
            target.scale = Qt.vector3d(1, 1, 1);
        } else {
            // Calculate the distance independent scale by first mapping the targets position to
            // the view. We then measure up a distance on the view (100px) that we use as an
            // "anchor" distance. Map the two positions back to the target node, and measure the
            // distance between them now, in the 3D scene. The difference of the two distances,
            // view and scene, will tell us what the distance independent scale should be.

            // Need to recreate vector as we need to adjust it and we can't do that on reference of
            // scenePosition, which is read-only property
            var scenePos = Qt.vector3d(scenePosition.x, scenePosition.y, scenePosition.z);
            if (orientation === Node.RightHanded)
                scenePos.z = -scenePos.z;

            var posInView1 = view3D.mapFrom3DScene(scenePos);
            var posInView2 = Qt.vector3d(posInView1.x + 100, posInView1.y, posInView1.z);

            var rayPos1 = view3D.mapTo3DScene(Qt.vector3d(posInView2.x, posInView2.y, 0));
            var rayPos2 = view3D.mapTo3DScene(Qt.vector3d(posInView2.x, posInView2.y, 10));

            var planeNormal = camera.forward;
            var rayHitPos = helper.rayIntersectsPlane(rayPos1, rayPos2, scenePos, planeNormal);
            relativeScale = scenePos.minus(rayHitPos).length() / 100;
        }
    }

    MouseArea3D {
        id: helper
        active: false
        view3D: overlayNode.view3D
    }
}
