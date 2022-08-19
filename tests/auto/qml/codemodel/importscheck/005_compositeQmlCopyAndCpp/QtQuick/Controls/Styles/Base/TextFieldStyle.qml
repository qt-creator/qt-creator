// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype TextFieldStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup controlsstyling
    \brief Provides custom styling for TextField.

    Example:
    \qml
    TextField {
        style: TextFieldStyle {
            textColor: "black"
            background: Rectangle {
                radius: 2
                implicitWidth: 100
                implicitHeight: 24
                border.color: "#333"
                border.width: 1
            }
        }
    }
    \endqml
*/

Style {
    id: style

    /*! \internal */
    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }
    /*! The \l TextField attached to this style. */
    readonly property TextField control: __control

    /*! The content margins of the text field. */
    padding { top: 4 ; left: TextSingleton.implicitHeight/3 ; right: TextSingleton.implicitHeight/3 ; bottom:4 }

    /*! The current font. */
    property font font

    /*! The text color. */
    property color textColor: __syspal.text

    /*! The text highlight color, used behind selections. */
    property color selectionColor: __syspal.highlight

    /*! The highlighted text color, used in selections. */
    property color selectedTextColor: __syspal.highlightedText

    /*!
        \qmlproperty enumeration renderType

        Override the default rendering type for the control.

        Supported render types are:
        \list
        \li Text.QtRendering
        \li Text.NativeRendering - the default
        \endlist

        \sa Text::renderType
    */
    property int renderType: Text.NativeRendering

    /*! The placeholder text color, used when the text field is empty.
        \since 5.2
    */
    property color placeholderTextColor: Qt.rgba(0, 0, 0, 0.5)

    /*! The background of the text field. */
    property Component background: Item {
        implicitWidth: Math.round(TextSingleton.implicitHeight * 8)
        implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -1
            color: "#44ffffff"
            radius: baserect.radius
        }
        Rectangle {
            id: baserect
            gradient: Gradient {
                GradientStop {color: "#e0e0e0" ; position: 0}
                GradientStop {color: "#fff" ; position: 0.1}
                GradientStop {color: "#fff" ; position: 1}
            }
            radius: TextSingleton.implicitHeight * 0.16
            anchors.fill: parent
            border.color: control.activeFocus ? "#47b" : "#999"
        }
    }

    /*! \internal */
    property Component panel: Item {
        anchors.fill: parent

        property int topMargin: padding.top
        property int leftMargin: padding.left
        property int rightMargin: padding.right
        property int bottomMargin: padding.bottom

        property color textColor: style.textColor
        property color selectionColor: style.selectionColor
        property color selectedTextColor: style.selectedTextColor

        implicitWidth: backgroundLoader.implicitWidth ? backgroundLoader.implicitWidth : 100
        implicitHeight: backgroundLoader.implicitHeight ? backgroundLoader.implicitHeight : 20

        property color placeholderTextColor: style.placeholderTextColor
        property font font: style.font

        Loader {
            id: backgroundLoader
            sourceComponent: background
            anchors.fill: parent
        }
    }
}
