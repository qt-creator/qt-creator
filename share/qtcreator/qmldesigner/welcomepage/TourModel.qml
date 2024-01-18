// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

ListModel {
    ListElement {
        qmlFileName: "tours/welcomepage-tour/WelcomeSlideShow.ui.qml"
        thumbnail: "images/welcome-page.png"
        title: QT_TR_NOOP("Welcome Page")
        subtitle: QT_TR_NOOP("The welcome page of Qt Design Studio.")
    }
    ListElement {
        qmlFileName: "tours/workspace-tour/WorkspaceSlideShow.ui.qml"
        thumbnail: "images/workspaces.png"
        title: QT_TR_NOOP("Workspaces")
        subtitle: QT_TR_NOOP("Introduction to the most important workspaces.")
    }
    ListElement {
        qmlFileName: "tours/toolbar-tour/ToolbarSlideShow.ui.qml"
        thumbnail: "images/top-toolbar.png"
        title: QT_TR_NOOP("Top Toolbar")
        subtitle: QT_TR_NOOP("Short explanation of the top toolbar.")
    }
    ListElement {
        qmlFileName: "tours/states-tour/StatesSlideShow.ui.qml"
        thumbnail: "images/states.png"
        title: QT_TR_NOOP("States")
        subtitle: QT_TR_NOOP("An introduction to states.")
    }
    ListElement {
        qmlFileName: "tours/sortcomponents-tour/SortComponentsSlideShow.ui.qml"
        thumbnail: "images/sorting-components.png"
        title: QT_TR_NOOP("Sorting Components")
        subtitle: QT_TR_NOOP("A way to organize multiple components.")
    }
    ListElement {
        qmlFileName: "tours/connectcomponents-tour/ConnectComponentsSlideShow.ui.qml"
        thumbnail: "images/connecting-components.png"
        title: QT_TR_NOOP("Connecting Components")
        subtitle: QT_TR_NOOP("A way to connect components with actions.")
    }
    ListElement {
        qmlFileName: "tours/addassets-tour/AddAssetsSlideShow.ui.qml"
        thumbnail: "images/adding-assets.png"
        title: QT_TR_NOOP("Adding Assets")
        subtitle: QT_TR_NOOP("A way to add new assets to the project.")
    }
    ListElement {
        qmlFileName: "tours/animation-tour/AnimationSlideShow.ui.qml"
        thumbnail: "images/animation-2d.png"
        title: QT_TR_NOOP("Creating 2D Animation")
        subtitle: QT_TR_NOOP("A way to create a 2D Animation.")
    }
    ListElement {
        qmlFileName: "tours/studiocomponents-border-arc-tour/StudioComponentBorderArcSlideShow.ui.qml"
        thumbnail: "images/border-arc.png"
        title: QT_TR_NOOP("Border and Arc")
        subtitle: QT_TR_NOOP("Work with Border and Arc Studio Components.")
    }
    ListElement {
        qmlFileName: "tours/studiocomponents-ellipse-pie-tour/StudioComponentEllipsePieSlideShow.ui.qml"
        thumbnail: "images/ellipse-pie.png"
        title: QT_TR_NOOP("Ellipse and Pie")
        subtitle: QT_TR_NOOP("Work with Ellipse and Pie Studio Components.")
    }
    ListElement {
        qmlFileName: "tours/studiocomponents-polygon-triangle-rectangle-tour/StudioComponentPolygonTriangleRectangleSlideShow.ui.qml"
        thumbnail: "images/complex-shapes.png"
        title: QT_TR_NOOP("Complex Shapes")
        subtitle: QT_TR_NOOP("Work with Polygon, Triangle and Rectangle Studio Components.")
    }
}
