/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

VersionData {
    name: "QUL 1.9"

    bannedItems: ["QtQuick.AnimatedImage",
        "QtQuick.FocusScope",
        "QtQuick.TextInput",
        "QtQuick.TextEdit",
        "QtQuick.Flow",
        "QtQuick.Grid",
        "QtQuick.GridView",
        "QtQuick.PathView",
        "QtQuick.Controls",
        "QtQuick.Controls.BusyIndicator",
        "QtQuick.Controls.ButtonGroup",
        "QtQuick.Controls.CheckDelegate",
        "QtQuick.Controls.Container",
        "QtQuick.Controls.ComboBox",
        "QtQuick.Controls.DelayButton",
        "QtQuick.Controls.Frame",
        "QtQuick.Controls.GroupBox",
        "QtQuick.Controls.ItemDelegate",
        "QtQuick.Controls.Label",
        "QtQuick.Controls.Page",
        "QtQuick.Controls.PageIndicator",
        "QtQuick.Controls.Pane",
        "QtQuick.Controls.RadioDelegate",
        "QtQuick.Controls.RangeSlider",
        "QtQuick.Controls.RoundButton",
        "QtQuick.Controls.ScrollView",
        "QtQuick.Controls.SpinBox",
        "QtQuick.Controls.StackView",
        "QtQuick.Controls.SwipeDelegate",
        "QtQuick.Controls.SwitchDelegate",
        "QtQuick.Controls.ToolBar",
        "QtQuick.Controls.ToolButton",
        "QtQuick.Controls.TabBar",
        "QtQuick.Controls.TabButton",
        "QtQuick.Controls.TextArea",
        "QtQuick.Controls.TextField",
        "QtQuick.Controls.ToolSeparator",
        "QtQuick.Controls.Tumbler"]

    allowedImports: ["QtQuick",
        "QtQuick.Shapes",
        "QtQuick.Controls",
        "QtQuick.Timeline",
        "QtQuickUltralite.Extras",
        "QtQuickUltralite.Layers"]

    bannedImports: ["FlowView"]

    //ComplexProperty is not a type, it's just a way to handle bigger props
    ComplexProperty {
        prefix: "font"
        bannedProperties: ["wordSpacing", "letterSpacing", "hintingPreference",
            "kerning", "preferShaping",  "capitalization",
            "strikeout", "underline", "styleName"]
    }

    QtQuick.Item {
        bannedProperties: ["layer", "opacity", "smooth", "antialiasing",
            "baselineOffset", "focus", "activeFocusOnTab",
            "rotation", "scale", "transformOrigin"]
    }

    QtQuick.Rectangle {
        bannedProperties: ["gradient", "border"]
    }

    QtQuick.Flickable {
        bannedProperties: ["boundsBehavior", "boundsMovement", "flickDeceleration",
            "flickableDirection", "leftMargin", "rightMargin", "bottomMargin", "topMargin",
            "originX", "originY", "pixelAligned", "pressDelay", "synchronousDrag"]
    }

    QtQuick.MouseArea {
        bannedProperties: ["propagateComposedEvents", "preventStealing", "cursorShape",
            "scrollGestureEnabled", "drag", "acceptedButtons", "hoverEnabled"]
    }

    QtQuick.Image {
        allowChildren: false
        allowedProperties: ["rotation", "scale", "transformOrigin"]
        bannedProperties: ["mirror", "mipmap",  "cache", "autoTransform", "asynchronous",
            "sourceSize", "smooth"]
    }

    QtQuick.BorderImage {
        bannedProperties: ["asynchronous", "cache", "currentFrame", "frameCount",
            "horizontalTileMode", "mirror", "progress", "smooth", "sourceSize",
            "status", "verticalTileMode"]
    }

    QtQuick.Text {
        allowChildren: false
        allowedProperties: ["rotation", "scale", "transformOrigin"]
        bannedProperties: ["elide", "lineHeight", "lineHeightMode", "wrapMode", "style",
            "styleColor", "minimumPointSize", "minimumPixelSize",
            "fontSizeMode", "renderType", "textFormat", "maximumLineCount"]
    }

    //Padding is not an actual item, but rather set of properties in Text
    Padding {
        bannedProperties: ["bottomPadding", "topPadding", "leftPadding", "rightPadding"]
    }

    QtQuick.Column {
        bannedProperties: ["bottomPadding", "leftPadding", "rightPadding", "topPadding"]
    }

    QtQuick.Row {
        bannedProperties: ["bottomPadding", "leftPadding", "rightPadding", "topPadding",
        "effectiveLayoutDirection", "layoutDirection"]
    }

    QtQuick.ListView {
        bannedProperties: ["cacheBuffer", "highlightRangeMode", "highlightMoveDuration",
            "highlightResizeDuration", "preferredHighlightBegin", "layoutDirection",
            "preferredHighlightEnd", "highlightFollowsCurrentItem", "keyNavigationWraps",
            "snapMode", "highlightMoveVelocity", "highlightResizeVelocity"]
    }

    QtQuick.Animation {
        bannedProperties: ["paused"]
    }

    //Quick Controls2 Items and properties:

    QtQuick.Controls.Control {
        bannedProperties: ["focusPolicy", "hoverEnabled", "wheelEnabled"]
    }

    QtQuick.Controls.AbstractButton {
        bannedProperties: ["display", "autoExclusive"]
    }

    QtQuick.Controls.ProgressBar {
        bannedProperties: ["indeterminate"]
    }

    QtQuick.Controls.Slider {
        bannedProperties: ["live", "snapMode", "touchDragThreshold"]
    }

    //Path and Shapes related:

    QtQuick.Path {
        bannedProperties: ["scale", "pathElements"]
    }

    QtQuick.PathArc {
        bannedProperties: ["relativeX", "relativeY"]
    }

    QtQuick.PathLine {
        bannedProperties: ["relativeX", "relativeY"]
    }

    QtQuick.PathMove {
        bannedProperties: ["relativeX", "relativeY"]
    }

    QtQuick.PathQuad {
        bannedProperties: ["relativeX", "relativeY",
            "relativeControlX", "relativeControlY"]
    }

    QtQuick.PathCubic {
        bannedProperties: ["relativeX", "relativeY",
            "relativeControl1X", "relativeControl1Y",
            "relativeControl2X", "relativeControl2Y"]
    }

    QtQuick.PathElement {
        //nothing
    }

    QtQuick.PathSvg {
        //nothing
    }

    QtQuick.Shapes.Shape {
        bannedProperties: ["asynchronous", "containsMode", "data",
            "renderType", "status", "vendorExtensionsEnabled"]
    }

    QtQuick.Shapes.ShapePath {
        bannedProperties: ["dashOffset", "dashPattern",
            "fillGradient", "strokeStyle"]
    }
}
