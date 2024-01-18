// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import WelcomeScreen 1.0
import StudioTheme 1.0 as StudioTheme

T.ScrollBar {
    id: control

    property bool show: false
    property bool otherInUse: false
    property bool isNeeded: control.size < 1.0
    property bool inUse: control.hovered || control.pressed
    property int thickness: control.inUse || control.otherInUse ? 10 : 8

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
        color: control.inUse ? Constants.currentScrollBarHandle
                             : Constants.currentScrollBarHandle_idle
    }

    background: Rectangle {
        id: controlTrack
        color: Constants.currentScrollBarTrack
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

    Behavior on x {
        PropertyAnimation {
            duration: 100
            easing.type: Easing.InOutQuad
        }
    }
}
