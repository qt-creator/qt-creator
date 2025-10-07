// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// new import: QUL.Studio.Components
// new types: ArcItem, Gradients, Basic and LinearGradient
// new properties: gradients related
// Layer Application renamed to ApplicationScreens

VersionData {
    name: "Qt for MCUs 2.7"

    bannedItems: [
        "QtQuick.AnimatedImage",
        "QtQuick.Flow",
        "QtQuick.FocusScope",
        "QtQuick.Grid",
        "QtQuick.GridView",
        "QtQuick.PathView",
        "QtQuick.TextEdit",
        "QtQuick.TextInput",
        "QtQuick.VectorImage.VectorImage",
        "QtQuick.Controls",
        "QtQuick.Controls.BusyIndicator",
        "QtQuick.Controls.ButtonGroup",
        "QtQuick.Controls.CheckDelegate",
        "QtQuick.Controls.ComboBox",
        "QtQuick.Controls.Container",
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
        "QtQuick.Controls.TabBar",
        "QtQuick.Controls.TabButton",
        "QtQuick.Controls.TextArea",
        "QtQuick.Controls.TextField",
        "QtQuick.Controls.ToolBar",
        "QtQuick.Controls.ToolButton",
        "QtQuick.Controls.ToolSeparator",
        "QtQuick.Controls.Tumbler",
        "QtQuick.Shapes.ConicalGradient",
        "QtQuick.Shapes.RadialGradient"
    ]

    allowedImports: [
        "QtQuick",
        "QtQuick.Controls",
        "QtQuick.Shapes",
        "QtQuick.Timeline",
        "QtQuickUltralite.Extras",
        "QtQuickUltralite.Layers",
        "QtQuickUltralite.Profiling",
        "QtQuickUltralite.Studio.Components"
    ]

    bannedImports: [
        "FlowView",
        "SimulinkConnector"
    ]

    //ComplexProperty is not a type, it's just a way to handle bigger props
    ComplexProperty {
        prefix: "font"
        bannedProperties: ["wordSpacing", "letterSpacing", "hintingPreference",
            "kerning", "preferShaping",  "capitalization",
            "strikeout", "underline", "styleName"]
    }

    QtQml.Timer {
        bannedProperties: ["triggeredOnStart"]
    }

    QtQuick.Item {
        bannedProperties: ["layer", "opacity", "smooth", "antialiasing",
            "baselineOffset", "focus", "activeFocusOnTab",
            "rotation", "scale", "transformOrigin"]
    }

    QtQuick.Rectangle {
        bannedProperties: ["border"]
    }

    QtQuick.Flickable {
        bannedProperties: ["boundsMovement", "flickDeceleration",
            "leftMargin", "rightMargin", "bottomMargin", "topMargin",
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
        bannedProperties: ["lineHeight", "lineHeightMode", "style",
            "styleColor", "minimumPointSize", "minimumPixelSize",
            "fontSizeMode", "renderType", "renderTypeQuality", "maximumLineCount"]
    }

    QtQuick.Loader {
        bannedProperties: ["asynchronous", "progress", "status"]
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

    QtQuick.AnimatedSprite {
        allowedProperties: ["currentFrame", "frameCount", "paused"]
        bannedProperties: ["finishBehavior", "frameRate", "frameSync",
            "frameX", "frameY", "interpolate", "reverse"]
    }

    //Quick Controls2 Items and properties:

    QtQuick.Controls.Control {
        bannedProperties: ["focusPolicy", "hoverEnabled", "wheelEnabled"]
    }

    QtQuick.Controls.AbstractButton {
        bannedProperties: ["display", "autoExclusive", "icon"]
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
        bannedProperties: ["dashOffset", "dashPattern", "strokeStyle"]
    }

    QtQuickUltralite.Extras.ItemBuffer {
        allowedProperties: ["rotation", "scale", "transformOrigin"]
    }
}
