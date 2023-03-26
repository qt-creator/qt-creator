// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0

Item {
    id: root
    width: 150
    height: 150
    visible: true

    property View3D view: null
    property alias contentItem: contentItem

    property var previewObject

    property var materialViewComponent
    property var modelViewComponent
    property var nodeViewComponent

    property bool closeUp: false

    function destroyView()
    {
        previewObject = null;
        if (view)
            view.destroy();
    }

    function createViewForObject(obj, env, envValue, model)
    {
        backgroundView3d.visible = true;
        if (obj instanceof Material)
            createViewForMaterial(obj, env, envValue, model);
        else if (obj instanceof Model)
            createViewForModel(obj);
        else if (obj instanceof Node)
            createViewForNode(obj);
    }

    function createViewForMaterial(material, env, envValue, model)
    {
        if (!materialViewComponent)
            materialViewComponent = Qt.createComponent("MaterialNodeView.qml");

        // Always recreate the view to ensure material is up to date
        if (materialViewComponent.status === Component.Ready) {
            view = materialViewComponent.createObject(viewRect, {"previewMaterial": material,
                                                                 "envMode": env,
                                                                 "envValue": envValue,
                                                                 "modelSrc": model});
        }
        // Floor must be in same view as material so material can reflect it, so hide it in this one
        backgroundView3d.visible = false;
        previewObject = material;
    }

    function createViewForModel(model)
    {
        if (!modelViewComponent)
            modelViewComponent = Qt.createComponent("ModelNodeView.qml");

        // Always recreate the view to ensure model is up to date
        if (modelViewComponent.status === Component.Ready)
            view = modelViewComponent.createObject(viewRect, {"sourceModel": model});

        previewObject = model;
    }

    function createViewForNode(node)
    {
        if (!nodeViewComponent)
            nodeViewComponent = Qt.createComponent("NodeNodeView.qml");

        // Always recreate the view to ensure node is up to date
        if (nodeViewComponent.status === Component.Ready)
            view = nodeViewComponent.createObject(viewRect, {"importScene": node});

        previewObject = node;
    }

    function fitToViewPort()
    {
        view.fitToViewPort(closeUp);
    }

    // Enables/disables icon mode. When in icon mode, camera is zoomed bit closer to reduce margins
    // and the background is removed, in order to generate a preview suitable for library icons.
    function setIconMode(enable)
    {
        closeUp = enable;
        backgroundRect.visible = !enable;
    }

    Item {
        id: contentItem
        anchors.fill: parent

        Item {
            id: viewRect
            anchors.fill: parent
        }

        Rectangle {
            id: backgroundRect
            anchors.fill: parent
            z: -1
            gradient: Gradient {
                GradientStop { position: 1.0; color: "#222222" }
                GradientStop { position: 0.0; color: "#999999" }
            }

            // Use View3D instead of static image to make background look good on all resolutions
            View3D {
                id: backgroundView3d
                anchors.fill: parent
                environment: sceneEnv

                SceneEnvironment {
                    id: sceneEnv
                    antialiasingMode: SceneEnvironment.MSAA
                    antialiasingQuality: SceneEnvironment.High
                }

                DirectionalLight {
                    eulerRotation.x: -26
                    eulerRotation.y: -57
                }

                PerspectiveCamera {
                    y: 125
                    z: 120
                    eulerRotation.x: -31
                    clipNear: 1
                    clipFar: 1000
                }

                Model {
                    id: floorModel
                    source: "#Rectangle"
                    scale.y: 8
                    scale.x: 8
                    eulerRotation.x: -90
                    materials: floorMaterial
                    DefaultMaterial {
                        id: floorMaterial
                        diffuseMap: floorTex
                        Texture {
                            id: floorTex
                            source: "../images/floor_tex.png"
                            scaleU: floorModel.scale.x
                            scaleV: floorModel.scale.y
                        }
                    }
                }
            }
        }
    }
}
