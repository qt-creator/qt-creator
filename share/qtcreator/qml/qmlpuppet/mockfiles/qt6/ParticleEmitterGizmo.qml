/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick3D
import QtQuick3D.Particles3D

// Note: This gizmo is also used to visualize Attractor3D in addition to ParticleEmitter3D,
//       as the two are very similar.

Node {
    id: root

    property Node targetNode: null
    property var selectedNodes: []
    property Node activeParticleSystem: null
    property Node scene: null
    property Node activeScene: null
    property bool hidden: false
    property bool systemHidden: false
    property Node shapeModel: null
    property bool globalShow: false
    property bool canBeVisible: activeScene === scene && targetNode && !hidden && !systemHidden
    property bool partOfActiveSystem: root.targetNode && root.targetNode.system === activeParticleSystem
    property bool isEmitter: targetNode && targetNode instanceof ParticleEmitter3D

    opacity: 0.15

    readonly property bool selected: selectedNodes.includes(targetNode)

    visible: canBeVisible && (globalShow || selected)

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    rotation: targetNode ? targetNode.sceneRotation : Qt.quaternion(1, 0, 0, 0)
    scale: targetNode ? targetNode.sceneScale : Qt.vector3d(1, 1, 1)

    function basicShape()
    {
        if (targetNode && targetNode.shape && targetNode.shape instanceof ParticleShape3D) {
            if (targetNode.shape.type === ParticleShape3D.Cube)
                return "#Cube";
            else if (targetNode.shape.type === ParticleShape3D.Cylinder)
                return "#Cylinder";
        }
        return "#Sphere";
    }

    function updateShape()
    {
        if (shapeModel)
            shapeModel.destroy();

        if (!targetNode)
            return;

        if (targetNode.shape instanceof ParticleModelShape3D) {
            shapeModel = _generalHelper.createParticleEmitterGizmoModel(targetNode, defaultMaterial);
            shapeModel.parent = root;
        }
    }

    Component.onCompleted: {
        updateShape();
    }

    Connections {
        target: targetNode
        function onSystemChanged() { systemHidden = _generalHelper.isHidden(system); }
    }

    Connections {
        target: targetNode
        function onShapeChanged() { updateShape(); }
    }

    Connections {
        target: targetNode.shape instanceof ParticleModelShape3D ? targetNode.shape
                                                                 :null
        function onDelegateChanged() { updateShape(); }
    }

    Connections {
        target: targetNode.shape instanceof ParticleModelShape3D ? targetNode.shape.delegate
                                                                 : null
        function onSourceChanged() { updateShape(); }
    }

    Model {
        readonly property Node _pickTarget: root.targetNode
        materials: [defaultMaterial]
        source: basicShape()
        scale: root.targetNode && root.targetNode.shape && targetNode.shape instanceof ParticleShape3D
               ? root.targetNode.shape.extents.times(0.02) // default extent is 50
               : autoScale.getScale(Qt.vector3d(0.1, 0.1, 0.1))
        visible: !shapeModel
    }

    AutoScaleHelper {
        id: autoScale
        view3D: overlayView
    }

    DefaultMaterial {
        id: defaultMaterial
        diffuseColor: root.selected ? "#FF0000"
                                    : root.partOfActiveSystem
                                      ? root.isEmitter ? "#FFFF00" : "#0000FF"
                                      : "#AAAAAA"
        lighting: DefaultMaterial.NoLighting
        cullMode: Material.NoCulling
    }
}
