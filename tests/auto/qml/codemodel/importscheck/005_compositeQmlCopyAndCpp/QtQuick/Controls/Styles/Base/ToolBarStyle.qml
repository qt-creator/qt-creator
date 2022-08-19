// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ToolBarStyle
    \inqmlmodule QtQuick.Controls.Styles
    \ingroup controlsstyling
    \since 5.2
    \brief Provides custom styling for ToolBar

    The tool bar can be defined by overriding the background component and
    setting the content padding.

    Example:
    \qml
    ToolBar {
        style: ToolBarStyle {
            padding {
                left: 8
                right: 8
                top: 3
                bottom: 3
            }
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 40
                border.color: "#999"
                gradient: Gradient {
                    GradientStop { position: 0 ; color: "#fff" }
                    GradientStop { position: 1 ; color: "#eee" }
                }
            }
        }
    }
    \endqml
*/

Style {

    /*! The content padding inside the tool bar. */
    padding {
        left: 6
        right: 6
        top: 3
        bottom: 3
    }

    /*! This defines the background of the tool bar. */
    property Component background: Item {
        implicitHeight: 40
        implicitWidth: 200
        Rectangle {
            anchors.fill: parent
            gradient: Gradient{
                GradientStop{color: "#eee" ; position: 0}
                GradientStop{color: "#ccc" ; position: 1}
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#999"
            }
        }
    }

    property Component panel: Loader {
        sourceComponent: background
    }
}
