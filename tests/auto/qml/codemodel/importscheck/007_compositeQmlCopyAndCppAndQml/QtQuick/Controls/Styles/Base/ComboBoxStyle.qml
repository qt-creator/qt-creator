// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ComboBoxStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup controlsstyling
    \brief Provides custom styling for ComboBox
*/

Style {

    /*! \internal */
    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }
    /*! The \l ComboBox attached to this style. */
    readonly property ComboBox control: __control

    /*! The padding between the background and the label components. */
    padding { top: 4 ; left: 6 ; right: 6 ; bottom:4 }

    /*! The size of the drop down button when the combobox is editable. */
    property int drowDownButtonWidth: Math.round(TextSingleton.implicitHeight)

    /*! This defines the background of the button. */
    property Component background: Item {
        implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
        implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: control.pressed ? 0 : -1
            color: "#10000000"
            radius: baserect.radius
        }
        Rectangle {
            id: baserect
            gradient: Gradient {
                GradientStop {color: control.pressed ? "#bababa" : "#fefefe" ; position: 0}
                GradientStop {color: control.pressed ? "#ccc" : "#e3e3e3" ; position: 1}
            }
            radius: TextSingleton.implicitHeight * 0.16
            anchors.fill: parent
            border.color: control.activeFocus ? "#47b" : "#999"
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: control.activeFocus ? "#47b" : "white"
                opacity: control.hovered || control.activeFocus ? 0.1 : 0
                Behavior on opacity {NumberAnimation{ duration: 100 }}
            }
        }
        Image {
            id: imageItem
            visible: control.menu !== null
            source: "images/arrow-down.png"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: drowDownButtonWidth / 2
            opacity: control.enabled ? 0.6 : 0.3
        }
    }

    /*! \internal */
    property Component __editor: Item {
        implicitWidth: 100
        implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))
        clip: true
        Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: 0
            color: "#44ffffff"
            radius: baserect.radius
        }
        Rectangle {
            id: baserect
            anchors.rightMargin: -radius
            anchors.bottomMargin: 1
            gradient: Gradient {
                GradientStop {color: "#e0e0e0" ; position: 0}
                GradientStop {color: "#fff" ; position: 0.1}
                GradientStop {color: "#fff" ; position: 1}
            }
            radius: TextSingleton.implicitHeight * 0.16
            anchors.fill: parent
            border.color: control.activeFocus ? "#47b" : "#999"
        }
        Rectangle {
            color: "#aaa"
            anchors.bottomMargin: 2
            anchors.topMargin: 1
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
        }
    }

    /*! This defines the label of the button. */
    property Component label: Item {
        implicitWidth: textitem.implicitWidth + 20
        Text {
            id: textitem
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 4
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: control.currentText
            renderType: Text.NativeRendering
            color: __syspal.text
            elide: Text.ElideRight
        }
    }

    /*! \internal */
    property Component panel: Item {
        property bool popup: false
        anchors.centerIn: parent
        anchors.fill: parent
        implicitWidth: backgroundLoader.implicitWidth
        implicitHeight: Math.max(labelLoader.implicitHeight + padding.top + padding.bottom, backgroundLoader.implicitHeight)

        Loader {
            id: backgroundLoader
            anchors.fill: parent
            sourceComponent: background

        }

        Loader {
            id: editorLoader
            anchors.fill: parent
            anchors.rightMargin: drowDownButtonWidth + padding.right
            anchors.bottomMargin: -1
            sourceComponent: control.editable ? __editor : null
        }

        Loader {
            id: labelLoader
            sourceComponent: label
            visible: !control.editable
            anchors.fill: parent
            anchors.leftMargin: padding.left
            anchors.topMargin: padding.top
            anchors.rightMargin: padding.right
            anchors.bottomMargin: padding.bottom
        }
    }

    /*! \internal */
    property Component __dropDownStyle: MenuStyle {
        maxPopupHeight: 600
        __menuItemType: "comboboxitem"
        scrollerStyle: ScrollViewStyle {
            property bool useScrollers: false
        }
    }

    /*! \internal */
    property Component __popupStyle: Style {

        property Component frame: Rectangle {
            width: (parent ? parent.contentWidth : 0)
            height: (parent ? parent.contentHeight : 0) + 2
            border.color: "white"
            property real maxHeight: 500
            property int margin: 1
        }

        property Component menuItem: Text {
            text: "NOT IMPLEMENTED"
            color: "red"
            font {
                pixelSize: 14
                bold: true
            }
        }

        property Component scrollerStyle: Style {
            padding { left: 0; right: 0; top: 0; bottom: 0 }
            property bool scrollToClickedPosition: false
            property Component frame: Item { visible: false }
            property Component corner: Item { visible: false }
            property Component __scrollbar: Item { visible: false }
            property bool useScrollers: true
        }
    }
}
