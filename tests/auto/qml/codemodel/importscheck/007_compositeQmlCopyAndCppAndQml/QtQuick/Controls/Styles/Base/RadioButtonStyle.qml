// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype RadioButtonStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup controlsstyling
    \brief Provides custom styling for RadioButton

    Example:
    \qml
    RadioButton {
        text: "Radio Button"
        style: RadioButtonStyle {
            indicator: Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 9
                    border.color: control.activeFocus ? "darkblue" : "gray"
                    border.width: 1
                    Rectangle {
                        anchors.fill: parent
                        visible: control.checked
                        color: "#555"
                        radius: 9
                        anchors.margins: 4
                    }
            }
        }
     }
    \endqml
*/

Style {
    id: radiobuttonStyle

    /*! \internal */
    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }
    /*! The \l RadioButton attached to this style. */
    readonly property RadioButton control: __control

    /*! This defines the text label. */
    property Component label: Item {
        implicitWidth: text.implicitWidth + 2
        implicitHeight: text.implicitHeight
        Rectangle {
            anchors.fill: text
            anchors.margins: -1
            anchors.leftMargin: -3
            anchors.rightMargin: -3
            visible: control.activeFocus
            height: 6
            radius: 3
            color: "#224f9fef"
            border.color: "#47b"
            opacity: 0.6
        }
        Text {
            id: text
            text: control.text
            anchors.centerIn: parent
            color: __syspal.text
            renderType: Text.NativeRendering
        }
    }

    /*! The background under indicator and label. */
    property Component background

    /*! The spacing between indicator and label. */
    property int spacing: Math.round(TextSingleton.implicitHeight/4)

    /*! This defines the indicator button.  */
    property Component indicator: Rectangle {
        width:  Math.round(TextSingleton.implicitHeight)
        height: width
        gradient: Gradient {
            GradientStop {color: "#eee" ; position: 0}
            GradientStop {color: control.pressed ? "#eee" : "#fff" ; position: 0.4}
            GradientStop {color: "#fff" ; position: 1}
        }
        border.color: control.activeFocus ? "#16c" : "gray"
        antialiasing: true
        radius: height/2
        Rectangle {
            anchors.centerIn: parent
            width: Math.round(parent.width * 0.5)
            height: width
            gradient: Gradient {
                GradientStop {color: "#999" ; position: 0}
                GradientStop {color: "#555" ; position: 1}
            }
            border.color: "#222"
            antialiasing: true
            radius: height/2
            Behavior on opacity {NumberAnimation {duration: 80}}
            opacity: control.checked ? control.enabled ? 1 : 0.5 : 0
        }
    }

    /*! \internal */
    property Component panel: Item {
        implicitWidth: Math.max(backgroundLoader.implicitWidth, row.implicitWidth + padding.left + padding.right)
        implicitHeight: Math.max(backgroundLoader.implicitHeight, labelLoader.implicitHeight + padding.top + padding.bottom,indicatorLoader.implicitHeight + padding.top + padding.bottom)

        Loader {
            id:backgroundLoader
            sourceComponent: background
            anchors.fill: parent
        }
        Row {
            id: row
            anchors.fill: parent

            anchors.leftMargin: padding.left
            anchors.rightMargin: padding.right
            anchors.topMargin: padding.top
            anchors.bottomMargin: padding.bottom

            spacing: radiobuttonStyle.spacing
            Loader {
                id: indicatorLoader
                sourceComponent: indicator
                anchors.verticalCenter: parent.verticalCenter
            }
            Loader {
                id: labelLoader
                sourceComponent: label
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
