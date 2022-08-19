// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    id: root

    padding {
        property int frameWidth: __styleitem.pixelMetric("defaultframewidth")
        left: frameWidth
        top: frameWidth
        bottom: frameWidth
        right: frameWidth
    }

    property StyleItem __styleitem: StyleItem { elementType: "frame" }

    property Component frame: StyleItem {
        id: styleitem
        elementType: "frame"
        sunken: true
        visible: control.frameVisible
        textureHeight: 64
        textureWidth: 64
        border {
            top: 16
            left: 16
            right: 16
            bottom: 16
        }
    }

    property Component corner: StyleItem { elementType: "scrollareacorner" }

    readonly property bool __externalScrollBars: __styleitem.styleHint("externalScrollBars")
    readonly property int __scrollBarSpacing: __styleitem.pixelMetric("scrollbarspacing")
    readonly property bool scrollToClickedPosition: __styleitem.styleHint("scrollToClickPosition") !== 0

    property Component __scrollbar: StyleItem {
        anchors.fill:parent
        elementType: "scrollbar"
        hover: activeControl != "none"
        activeControl: "none"
        sunken: __styleData.upPressed | __styleData.downPressed | __styleData.handlePressed
        minimum: __control.minimumValue
        maximum: __control.maximumValue
        value: __control.value
        horizontal: __styleData.horizontal
        enabled: __control.enabled

        implicitWidth: horizontal ? 200 : pixelMetric("scrollbarExtent")
        implicitHeight: horizontal ? pixelMetric("scrollbarExtent") : 200
    }

}
