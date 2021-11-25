/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    property bool ready: false

    function destroyView()
    {
        previewObject = null;
        if (view)
            view.destroy();
    }

    function createViewForObject(obj)
    {
        if (obj instanceof Material)
            createViewForMaterial(obj);
        else if (obj instanceof Model)
            createViewForModel(obj);
        else if (obj instanceof Node)
            createViewForNode(obj);

        previewObject = obj;
    }

    function createViewForMaterial(material)
    {
        if (!materialViewComponent)
            materialViewComponent = Qt.createComponent("MaterialNodeView.qml");

        // Always recreate the view to ensure material is up to date
        if (materialViewComponent.status === Component.Ready)
            view = materialViewComponent.createObject(viewRect, {"previewMaterial": material});
    }

    function createViewForModel(model)
    {
        if (!modelViewComponent)
            modelViewComponent = Qt.createComponent("ModelNodeView.qml");

        // Always recreate the view to ensure model is up to date
        if (modelViewComponent.status === Component.Ready)
            view = modelViewComponent.createObject(viewRect, {"sourceModel": model});
    }

    function createViewForNode(node)
    {
        if (!nodeViewComponent)
            nodeViewComponent = Qt.createComponent("NodeNodeView.qml");

        // Always recreate the view to ensure node is up to date
        if (nodeViewComponent.status === Component.Ready)
            view = nodeViewComponent.createObject(viewRect, {"importScene": node});
    }

    function afterRender()
    {
        if (previewObject instanceof Node) {
            view.fitToViewPort();
            ready = view.ready;
        } else {
            ready = true;
        }
    }

    Item {
        id: contentItem
        anchors.fill: parent

        Rectangle {
            id: viewRect
            anchors.fill: parent

            gradient: Gradient {
                GradientStop { position: 1.0; color: "#222222" }
                GradientStop { position: 0.0; color: "#999999" }
            }
        }
    }
}
