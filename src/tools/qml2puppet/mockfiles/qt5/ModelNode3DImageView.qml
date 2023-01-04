// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick3D 1.15

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

    View3D {
        // Dummy view to hold the context
        // TODO remove when QTBUG-87678 is fixed
    }

    Item {
        id: contentItem
        anchors.fill: parent

        Item {
            id: viewRect
            anchors.fill: parent
        }

        // We can use static image in Qt5 as only small previews will be generated
        Image {
            id: backgroundRect
            anchors.fill: parent
            z: -1
            source: "../images/static_floor.png"
            fillMode: Image.Stretch
        }
    }
}
