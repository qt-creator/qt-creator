// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

SplitView {
    id: root

    property string previewEnv
    property string previewModel

    property real __spacing: 5

    property StudioTheme.ControlStyle buttonStyle: StudioTheme.ViewBarButtonStyle {
        //This is how you can override stuff from the control styles
        baseIconFontSize: StudioTheme.Values.bigIconFontSize
    }

    function refreshPreview()
    {
        materialPreview.source = ""
        materialPreview.source = "image://materialEditor/preview"
    }

    // Called from C++ to close context menu on focus out
    function closeContextMenu()
    {
        modelMenu.close()
        envMenu.close()
    }

    anchors.left: parent.left
    anchors.right: parent.right

    implicitHeight: previewRect.implicitHeight + nameSection.implicitHeight

    orientation: Qt.Vertical

    handle: Rectangle {
        implicitWidth: root.orientation === Qt.Horizontal ? StudioTheme.Values.splitterThickness : root.width
        implicitHeight: root.orientation === Qt.Horizontal ? root.height : StudioTheme.Values.splitterThickness
        color: SplitHandle.pressed ? StudioTheme.Values.themeSliderHandleInteraction
            : (SplitHandle.hovered ? StudioTheme.Values.themeSliderHandleHover
                                     : "transparent")
    }

    StudioControls.Menu {
        id: modelMenu
        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        ListModel {
            id: modelMenuModel
            ListElement {
                modelName: qsTr("Cone")
                modelStr: "#Cone"
            }
            ListElement {
                modelName: qsTr("Cube")
                modelStr: "#Cube"
            }
            ListElement {
                modelName: qsTr("Cylinder")
                modelStr: "#Cylinder"
            }
            ListElement {
                modelName: qsTr("Sphere")
                modelStr: "#Sphere"
            }
        }

        Repeater {
            model: modelMenuModel
            StudioControls.MenuItemWithIcon {
                text: modelName
                onClicked: {
                    // Force property change notifications to keep check mark when reselected
                    root.previewModel = ""
                    root.previewModel = modelStr
                }
                checkable: true
                checked: root.previewModel === modelStr
            }
        }
    }

    StudioControls.Menu {
        id: envMenu
        closePolicy: StudioControls.Menu.CloseOnEscape | StudioControls.Menu.CloseOnPressOutside

        ListModel {
            id: envMenuModel
            ListElement {
                envName: qsTr("Basic")
                envStr: "Basic"
            }
            ListElement {
                envName: qsTr("Color")
                envStr: "Color"
            }
            ListElement {
                envName: qsTr("Studio")
                envStr: "SkyBox=preview_studio"
            }
            ListElement {
                envName: qsTr("Landscape")
                envStr: "SkyBox=preview_landscape"
            }
        }

        Repeater {
            model: envMenuModel
            StudioControls.MenuItemWithIcon {
                text: envName
                onClicked: {
                    // Force property change notifications to keep check mark when reselected
                    root.previewEnv = ""
                    root.previewEnv = envStr
                }
                checkable: true
                checked: root.previewEnv === envStr
            }
        }
    }

    Rectangle {
        id: previewRect

        SplitView.fillWidth: true
        SplitView.minimumWidth: 152
        SplitView.preferredHeight: Math.min(root.width * 0.75, 400)
        SplitView.minimumHeight: 150
        implicitHeight: materialPreview.height

        clip: true
        color: "#000000"

        Image {
            id: materialPreview

            width: root.width
            height: previewRect.height
            anchors.centerIn: parent

            fillMode: Image.PreserveAspectFit

            source: "image://materialEditor/preview"
            cache: false
            smooth: true

            sourceSize.width: materialPreview.width
            sourceSize.height: materialPreview.height

            Rectangle {
                id: toolbarRect

                radius: 10
                color: StudioTheme.Values.themeToolbarBackground
                width: optionsToolbar.width + 2 * toolbarRect.radius
                height: optionsToolbar.height + toolbarRect.radius
                anchors.left: parent.left
                anchors.leftMargin: -toolbarRect.radius
                anchors.verticalCenter: parent.verticalCenter

                Column {
                    id: optionsToolbar

                    spacing: root.__spacing
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: root.__spacing

                    HelperWidgets.AbstractButton {
                        id: pinButton

                        style: buttonStyle
                        buttonIcon: pinButton.checked ? StudioTheme.Constants.pin : StudioTheme.Constants.unpin
                        checkable: true
                        checked: itemPane.headerDocked
                        onCheckedChanged: itemPane.headerDocked = pinButton.checked
                    }

                    HelperWidgets.AbstractButton {
                        id: previewEnvMenuButton

                        style: buttonStyle
                        buttonIcon: StudioTheme.Constants.textures_medium
                        tooltip: qsTr("Select preview environment.")
                        onClicked: envMenu.popup()
                    }

                    HelperWidgets.AbstractButton {
                        id: previewModelMenuButton

                        style: buttonStyle
                        buttonIcon: StudioTheme.Constants.cube_medium
                        tooltip: qsTr("Select preview model.")
                        onClicked: modelMenu.popup()
                    }
                }
            }
        }
    }

    HelperWidgets.Section {
        id: nameSection

        // Section with hidden header is used so properties are aligned with the other sections' properties
        hideHeader: true
        SplitView.fillWidth: true
        SplitView.preferredHeight: implicitHeight
        SplitView.maximumHeight: implicitHeight
        bottomPadding: StudioTheme.Values.sectionPadding * 2
        collapsible: false

        HelperWidgets.SectionLayout {
            HelperWidgets.PropertyLabel { text: qsTr("Name") }

            HelperWidgets.SecondColumnLayout {
                HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                HelperWidgets.LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    width: StudioTheme.Values.singleControlColumnWidth
                    backendValue: backendValues.objectName
                    placeholderText: qsTr("Material name")

                    text: backendValues.id.value
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false

                    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                    validator: RegularExpressionValidator { regularExpression: /^(\w+\s)*\w+$/ }
                }

                HelperWidgets.ExpandingSpacer {}
            }

            HelperWidgets.PropertyLabel { text: qsTr("Type") }

            HelperWidgets.SecondColumnLayout {
                HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                HelperWidgets.ComboBox {
                    currentIndex: possibleTypeIndex
                    model: possibleTypes
                    showExtendedFunctionButton: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    enabled: possibleTypes.length > 1

                    onActivated: changeTypeName(currentValue)
                }
            }
        }
    }
}
