// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import QtQuick.Timeline 1.0
import welcome 1.0
import StudioFonts 1.0

Item {
    id: root
    visible: true
    width: 270
    height: 175
    property alias imageSource: image.source
    property alias labelText: label.text

    property alias downloadIcon: downloadCloud.visible

    signal clicked()

    onVisibleChanged: {
        animateOpacity.start()
        animateScale.start()
    }

    NumberAnimation {
        id: animateOpacity
        property: "opacity"
        from: 0
        to: 1.0
        duration: 400
    }
    NumberAnimation {
        id: animateScale
        property: "scale"
        from: 0
        to: 1.0
        duration: 400
    }

    Rectangle {
        id: rectangle
        x: 0
        y: 0
        width: 270
        height: 146

        MouseArea {
            x: 17
            y: 12
            height: 125
            anchors.bottomMargin: -label.height
            anchors.fill: parent
            hoverEnabled: true
            onHoveredChanged: {
                if (saturationEffect.desaturation === 1)
                    saturationEffect.desaturation = 0
                if (saturationEffect.desaturation === 0)
                    saturationEffect.desaturation = 1
                if (saturationEffect.desaturation === 0)
                    rectangle.color = "#262728"
                if (saturationEffect.desaturation === 1)
                    rectangle.color = "#404244"
                if (saturationEffect.desaturation === 0)
                    label.color = "#686868"
                if (saturationEffect.desaturation === 1)
                    label.color = Constants.textDefaultColor
            }

            onExited: {
                saturationEffect.desaturation = 1
                rectangle.color = "#262728"
                label.color = "#686868"
            }

            onClicked: root.clicked()
        }
    }

    SaturationEffect {
        id: saturationEffect
        x: 15
        y: 10
        width: 240
        height: 125
        desaturation: 0
        antialiasing: true
        Behavior on desaturation {
            PropertyAnimation {
            }
        }

        Image {
            id: image
            width: 240
            height: 125
            mipmap: true
            fillMode: Image.PreserveAspectFit
        }
    }

    Timeline {
        id: animation
        startFrame: 0
        enabled: true
        endFrame: 1000

        KeyframeGroup {
            target: saturationEffect
            property: "desaturation"

            Keyframe {
                frame: 0
                value: 1
            }

            Keyframe {
                frame: 1000
                value: 0
            }
        }

        KeyframeGroup {
            target: label
            property: "color"

            Keyframe {
                value: "#686868"
                frame: 0
            }

            Keyframe {
                value: Constants.textDefaultColor
                frame: 1000
            }
        }

        KeyframeGroup {
            target: rectangle
            property: "color"

            Keyframe {
                value: "#262728"
                frame: 0
            }

            Keyframe {
                value: "#404244"
                frame: 1000
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: animation
        property: "currentFrame"
        running: false
        duration: 1000
        to: animation.endFrame
        from: animation.startFrame
        loops: 1
    }

    Text {
        id: label
        x: 1
        y: 145
        color: "#686868"

        renderType: Text.NativeRendering
        font.pixelSize: 14
        font.family: StudioFonts.titilliumWeb_regular
    }

    Image {
        id: downloadCloud
        x: 210
        y: 118
        width: 60
        height: 60
        source: "images/downloadCloud.svg"
        sourceSize.height: 60
        sourceSize.width: 60
        fillMode: Image.PreserveAspectFit
        visible: false
    }
}

/*##^##
Designer {
    D{i:0;formeditorZoom:1.3300000429153442}D{i:8}
}
##^##*/
