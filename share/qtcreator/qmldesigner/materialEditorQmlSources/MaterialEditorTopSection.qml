// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import QtQuick.Templates 2.15 as T
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Column {
    id: root

    signal toolBarAction(int action)

    property string previewEnv
    property string previewModel

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

    MaterialEditorToolBar {
        width: root.width

        onToolBarAction: (action) => root.toolBarAction(action)
    }

    Item { width: 1; height: 10 } // spacer


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

    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: previewRect.height

        Rectangle {
            id: previewRect
            anchors.horizontalCenter: parent.horizontalCenter
            width: 152
            height: 152
            color: "#000000"

            Image {
                id: materialPreview
                width: 150
                height: 150
                anchors.centerIn: parent
                source: "image://materialEditor/preview"
                cache: false
                smooth: true
            }
        }

        Item {
            id: previewOptions
            width: 40
            height: previewRect.height
            anchors.top: previewRect.top
            anchors.left: previewRect.right

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                IconButton {
                    icon: StudioTheme.Constants.materialPreviewEnvironment
                    iconSize: StudioTheme.Values.bigIconFontSize
                    buttonSize: previewOptions.width
                    tooltip: qsTr("Select preview environment.")
                    onClicked: envMenu.popup()
                }
                IconButton {
                    icon: StudioTheme.Constants.materialPreviewModel
                    iconSize: StudioTheme.Values.bigIconFontSize
                    buttonSize: previewOptions.width
                    tooltip: qsTr("Select preview model.")
                    onClicked: modelMenu.popup()
                }
            }
        }

    }

    Section {
        // Section with hidden header is used so properties are aligned with the other sections' properties
        hideHeader: true
        width: parent.width
        collapsible: false

        SectionLayout {
            PropertyLabel { text: qsTr("Name") }

            SecondColumnLayout {
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    width: StudioTheme.Values.singleControlColumnWidth
                    backendValue: backendValues.objectName
                    placeholderText: qsTr("Material name")

                    text: backendValues.id.value
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false

                    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                    validator: RegExpValidator { regExp: /^(\w+\s)*\w+$/ }
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Type") }

            SecondColumnLayout {
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                ComboBox {
                    currentIndex: possibleTypeIndex
                    model: possibleTypes
                    showExtendedFunctionButton: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth

                    onActivated: changeTypeName(currentValue)
                }
            }
        }
    }
}
