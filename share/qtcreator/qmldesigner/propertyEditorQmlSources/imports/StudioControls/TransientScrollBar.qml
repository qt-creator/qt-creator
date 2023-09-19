// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property bool show: false
    property bool otherInUse: false
    property bool isNeeded: control.size < 1.0
    property bool inUse: control.hovered || control.pressed
    property int thickness: control.inUse || control.otherInUse ? control.style.scrollBarThicknessHover
                                                                : control.style.scrollBarThickness

    property bool scrollBarVisible: parent.childrenRect.height > parent.height

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    hoverEnabled: true
    padding: 0
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    opacity: 0.0

    contentItem: Rectangle {
        implicitWidth: control.thickness
        implicitHeight: control.thickness
        radius: width / 2
        color: control.inUse ? control.style.scrollBar.handleHover : control.style.scrollBar.handle
    }

    background: Rectangle {
        id: controlTrack
        color: control.style.scrollBar.track
        opacity: control.inUse || control.otherInUse ? 0.3 : 0.0
        radius: width / 2

        Behavior on opacity {
            PropertyAnimation {
                duration: 100
                easing.type: Easing.InOutQuad
            }
        }
    }

    states: [
        State {
            name: "show"
            when: control.show
            PropertyChanges {
                target: control
                opacity: 1.0
            }
        },
        State {
            name: "hide"
            when: !control.show
            PropertyChanges {
                target: control
                opacity: 0.0
            }
        }
    ]

    transitions: Transition {
        from: "show"
        SequentialAnimation {
            PauseAnimation { duration: 450 }
            NumberAnimation {
                target: control
                duration: 200
                property: "opacity"
                to: 0.0
            }
        }
    }

    Behavior on thickness {
        PropertyAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
        }
    }
}
