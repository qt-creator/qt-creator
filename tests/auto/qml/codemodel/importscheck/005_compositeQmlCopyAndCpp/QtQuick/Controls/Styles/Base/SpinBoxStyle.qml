// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype SpinBoxStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.2
    \ingroup controlsstyling
    \brief Provides custom styling for SpinBox

    Example:
    \qml
    SpinBox {
        style: SpinBoxStyle{
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 20
                border.color: "gray"
                radius: 2
            }
        }
    }
    \endqml
*/

Style {
    id: spinboxStyle

    /*! The \l SpinBox attached to this style. */
    readonly property SpinBox control: __control

    /*! \internal */
    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }

    /*! The content margins of the text field. */
    padding { top: 1 ; left: Math.round(TextSingleton.implicitHeight/2) ; right: Math.round(TextSingleton.implicitHeight) ; bottom: 0 }

    /*! \qmlproperty enumeration horizontalAlignment

        This property defines the default text aligment.

        The supported values are:
        \list
        \li Qt.AlignLeft
        \li Qt.AlignHCenter
        \li Qt.AlignRight
        \endlist

        The default value is Qt.AlignRight
    */
    property int horizontalAlignment: Qt.AlignRight

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

    /*! The button used to increment the value. */
    property Component incrementControl: Item {
        implicitWidth: padding.right
        Image {
            source: "images/arrow-up.png"
            anchors.centerIn: parent
            anchors.verticalCenterOffset: 1
            opacity: control.enabled ? (styleData.upPressed ? 1 : 0.6) : 0.5
        }
    }

    /*! The button used to decrement the value. */
    property Component decrementControl: Item {
        implicitWidth: padding.right
        Image {
            source: "images/arrow-down.png"
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -2
            opacity: control.enabled ? (styleData.downPressed ? 1 : 0.6) : 0.5
        }
    }

    /*! The background of the SpinBox. */
    property Component background: Item {
        implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))
        implicitWidth: styleData.contentWidth + 26
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
        id: styleitem
        implicitWidth: backgroundLoader.implicitWidth
        implicitHeight: backgroundLoader.implicitHeight

        property color foregroundColor: spinboxStyle.textColor
        property color selectionColor: spinboxStyle.selectionColor
        property color selectedTextColor: spinboxStyle.selectedTextColor

        property var margins: spinboxStyle.padding

        property rect upRect: Qt.rect(width - incrementControlLoader.implicitWidth, 0, incrementControlLoader.implicitWidth, height / 2 + 1)
        property rect downRect: Qt.rect(width - decrementControlLoader.implicitWidth, height / 2, decrementControlLoader.implicitWidth, height / 2)

        property int horizontalAlignment: spinboxStyle.horizontalAlignment
        property int verticalAlignment: Qt.AlignVCenter

        Loader {
            id: backgroundLoader
            anchors.fill: parent
            sourceComponent: background
        }

        Loader {
            id: incrementControlLoader
            x: upRect.x
            y: upRect.y
            width: upRect.width
            height: upRect.height
            sourceComponent: incrementControl
        }

        Loader {
            id: decrementControlLoader
            x: downRect.x
            y: downRect.y
            width: downRect.width
            height: downRect.height
            sourceComponent: decrementControl
        }
    }
}
