// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme

/*
 A ComboBox of flags, it expects a list model as below

itemsModel: ListModel {
    ListElement {
        name: "..."
        flag: ...
    }
    ListElement {
        name: "..."
        flag: ...
    }
    ...
*/
StudioControls.CustomComboBox {
    id: root

    required property var itemsModel

    property variant backendValue

    property bool showExtendedFunctionButton: true

    Connections {
        id: backendValueConnection

        target: backendValue

        function onValueChangedQml() {
            let numSelected = 0
            let selectedItem = ""

            for (let i = 0; i < root.itemsModel.count; ++i) {
                let flag = root.itemsModel.get(i).flag
                let flagActive = root.backendValue.value & flag
                root.popupItem.itemAt(i).checked = flagActive

                if (flagActive) {
                    selectedItem = root.itemsModel.get(i).name
                    ++numSelected
                }
            }

            // update ComboBox text
            root.model = numSelected == 0 ? [qsTr("empty")]
                       : numSelected == 1 ? root.model = [selectedItem]
                                          : [qsTr("%1 items selected").arg(numSelected)]
        }
    }

    Component.onCompleted: backendValueConnection.onValueChangedQml()

    model: [qsTr("empty")]

    popupComponent: Column {
        padding: 5
        spacing: 2

        function itemAt(idx) {
            return repeater.itemAt(idx)
        }

        Row {
            spacing: 5

            HelperWidgets.Button {
                text: qsTr("Select All")
                width: 80

                onClicked: {
                    let allFlags = 0
                    for (let i = 0; i < root.itemsModel.count; ++i)
                        allFlags += root.itemsModel.get(i).flag

                    root.backendValue.value = allFlags
                }
            }
            HelperWidgets.Button {
                text: qsTr("Select None")
                width: 80

                onClicked: root.backendValue.value = 0
            }
        }

        Repeater {
            id: repeater

            model: root.itemsModel
            delegate: StudioControls.CheckBox {
                text: name
                actionIndicatorVisible: false

                onToggled: {
                    if (root.popupItem.itemAt(index).checked)
                        root.backendValue.value |= flag
                    else
                        root.backendValue.value &= ~flag
                }
            }
        }
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: root.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible

    actionIndicator.visible: root.showExtendedFunctionButton
}
