// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

/*
 A ComboBox of flags, it expects a list mode as below, first item will act as clear:

itemsModel: ListModel {
    ListElement {
        name: qsTr("Clear")
        flag: 0
    }
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

            for (let i = 1; i < root.itemsModel.count; ++i) {
                let flag = (root.backendValue.value >> (i - 1)) & 1;
                root.popupItem.itemAt(i).checked = flag

                if (flag) {
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

        Repeater {
            id: repeater

            model: root.itemsModel
            delegate: StudioControls.CheckBox {
                text: name
                actionIndicatorVisible: false
                checked: (root.backendValue.value >> (index - 1)) & 1

                onToggled: {
                    if (index === 0) { // Clear
                        root.popupItem.itemAt(0).checked = false
                        root.backendValue.value = 0
                    } else {
                        if (root.popupItem.itemAt(index).checked)
                            root.backendValue.value |= flag
                        else
                            root.backendValue.value &= ~flag
                    }
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
