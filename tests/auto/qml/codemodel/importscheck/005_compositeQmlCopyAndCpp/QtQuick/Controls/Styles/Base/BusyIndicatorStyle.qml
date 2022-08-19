// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype BusyIndicatorStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.2
    \ingroup controlsstyling
    \brief Provides custom styling for BusyIndicatorStyle

    You can create a busy indicator by replacing the "indicator" delegate
    of the BusyIndicatorStyle with a custom design.

    Example:
    \qml
    BusyIndicator {
        style: BusyIndicatorStyle
            indicator: Image {
                visible: control.running
                source: "spinner.png"
                NumberAnimation on rotation {
                    running: control.running
                    loops: Animation.Infinite
                    duration: 2000
                    from: 0 ; to: 360
                }
            }
        }
    }
    \endqml
*/
Style {
    id: indicatorstyle

    /*! The \l BusyIndicator attached to this style. */
    readonly property BusyIndicator control: __control

    /*! This defines the appearance of the busy indicator. */
    property Component indicator: Item {
        implicitWidth: 48
        implicitHeight: 48

        opacity: control.running ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 250 } }

        Image {
            anchors.centerIn: parent
            anchors.alignWhenCentered: true
            width: Math.min(parent.width, parent.height)
            height: width
            source: width <= 32 ? "images/spinner_small.png" :
                                  width >= 48 ? "images/spinner_large.png" :
                                                "images/spinner_medium.png"
            RotationAnimator on rotation {
                duration: 800
                loops: Animation.Infinite
                from: 0
                to: 360
            }
        }
    }

    /*! \internal */
    property Component panel: Item {
        anchors.fill: parent
        implicitWidth: indicatorLoader.implicitWidth
        implicitHeight: indicatorLoader.implicitHeight

        Loader {
            id: indicatorLoader
            sourceComponent: indicator
            anchors.centerIn: parent
            width: Math.min(parent.width, parent.height)
            height: width
        }
    }
}
