// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype StatusBarStyle
    \inqmlmodule QtQuick.Controls.Styles
    \ingroup controlsstyling
    \since 5.2
    \brief Provides custom styling for StatusBar

    The status bar can be defined by overriding the background component and
    setting the content padding.

    Example:
    \qml
    StatusBar {
        style: StatusBarStyle {
            padding {
                left: 8
                right: 8
                top: 3
                bottom: 3
            }
            background: Rectangle {
                implicitHeight: 16
                implicitWidth: 200
                gradient: Gradient{
                    GradientStop{color: "#eee" ; position: 0}
                    GradientStop{color: "#ccc" ; position: 1}
                }
                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: "#999"
                }
            }
        }
    }
    \endqml
*/

Style {

    /*! The content padding inside the status bar. */
    padding {
        left: 3
        right: 3
        top: 3
        bottom: 2
    }

    /*! This defines the background of the tool bar. */
    property Component background: Rectangle {
        implicitHeight: 16
        implicitWidth: 200

        gradient: Gradient{
            GradientStop{color: "#eee" ; position: 0}
            GradientStop{color: "#ccc" ; position: 1}
        }

        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: "#999"
        }
    }

    property Component panel: Loader {
        sourceComponent: background
    }
}
