// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Window 2.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype CheckBoxStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup controlsstyling
    \brief Provides custom styling for CheckBox

    Example:
    \qml
    CheckBox {
        text: "Check Box"
        style: CheckBoxStyle {
            indicator: Rectangle {
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 3
                    border.color: control.activeFocus ? "darkblue" : "gray"
                    border.width: 1
                    Rectangle {
                        visible: control.checked
                        color: "#555"
                        border.color: "#333"
                        radius: 1
                        anchors.margins: 4
                        anchors.fill: parent
                    }
            }
        }
    }
    \endqml
*/
Style {
    id: checkboxStyle

    /*! The \l CheckBox attached to this style. */
    readonly property CheckBox control: __control
    /*! \internal */
    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }

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

    /*! This defines the indicator button. */
    property Component indicator:  Item {
        implicitWidth: Math.round(TextSingleton.implicitHeight)
        height: width
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -1
            color: "#44ffffff"
            radius: baserect.radius
        }
        Rectangle {
            id: baserect
            gradient: Gradient {
                GradientStop {color: "#eee" ; position: 0}
                GradientStop {color: control.pressed ? "#eee" : "#fff" ; position: 0.1}
                GradientStop {color: "#fff" ; position: 1}
            }
            radius: TextSingleton.implicitHeight * 0.16
            anchors.fill: parent
            border.color: control.activeFocus ? "#47b" : "#999"
        }

        Image {
            source: "images/check.png"
            opacity: control.checkedState === Qt.Checked ? control.enabled ? 1 : 0.5 : 0
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 1
            Behavior on opacity {NumberAnimation {duration: 80}}
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: Math.round(baserect.radius)
            antialiasing: true
            gradient: Gradient {
                GradientStop {color: control.pressed ? "#555" : "#999" ; position: 0}
                GradientStop {color: "#555" ; position: 1}
            }
            radius: baserect.radius - 1
            anchors.centerIn: parent
            anchors.alignWhenCentered: true
            border.color: "#222"
            Behavior on opacity {NumberAnimation {duration: 80}}
            opacity: control.checkedState === Qt.PartiallyChecked ? control.enabled ? 1 : 0.5 : 0
        }
    }

    /*! \internal */
    property Component panel: Item {
        implicitWidth: Math.max(backgroundLoader.implicitWidth, row.implicitWidth + padding.left + padding.right)
        implicitHeight: Math.max(backgroundLoader.implicitHeight, labelLoader.implicitHeight + padding.top + padding.bottom,indicatorLoader.implicitHeight + padding.top + padding.bottom)

        Loader {
            id: backgroundLoader
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

            spacing: checkboxStyle.spacing
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
