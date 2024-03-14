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
        flag: "..."
    }
    ListElement {
        name: "..."
        flag: "..."
    }
    ...
*/
StudioControls.CustomComboBox {
    id: root

    required property var itemsModel

    property variant backendValue

    property bool showExtendedFunctionButton: true

    property string scope: "Qt" // flag prefix scope
    property string zeroFlag

    Connections {
        id: backendValueConnection

        target: backendValue

        function onValueChangedQml() {
            let selectedItem = ""

            let flags = root.backendValue.expression.split(/\s*\|\s*/).filter(Boolean)

            for (let i = 0; i < root.itemsModel.count; ++i) {
                let flag = root.scope + "." + root.itemsModel.get(i).flag
                let flagActive = flags.indexOf(flag) !== -1
                root.popupItem.itemAt(i).checked = flagActive

                if (flagActive)
                    selectedItem = root.itemsModel.get(i).name
            }

            // update ComboBox text
            let numSelected = flags.length
            if (flags.length > 0 && flags[0] === root.scope + "." + root.zeroFlag)
                --numSelected

            root.model = numSelected === 0 ? [qsTr("empty")]
                       : numSelected === 1 ? root.model = [selectedItem]
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
                    let allFlags = root.scope + "." + root.itemsModel.get(0).flag
                    for (let i = 1; i < root.itemsModel.count; ++i)
                        allFlags += " | " + root.scope + "." + root.itemsModel.get(i).flag

                    root.backendValue.expression = allFlags
                }
            }
            HelperWidgets.Button {
                text: qsTr("Select None")
                width: 80

                onClicked: {
                    if (root.zeroFlag)
                        root.backendValue.expression = root.scope + "." + root.zeroFlag
                    else
                        root.backendValue.resetValue()
                }
            }
        }

        Repeater {
            id: repeater

            model: root.itemsModel
            delegate: StudioControls.CheckBox {
                text: name
                actionIndicatorVisible: false

                onToggled: {
                    let flags = root.backendValue.expression.split(/\s*\|\s*/).filter(Boolean)
                    let scopedFlag = root.scope + "." + flag
                    let idx = flags.indexOf(scopedFlag)

                    if (root.popupItem.itemAt(index).checked) {
                        if (idx === -1)
                            flags.push(scopedFlag)
                    } else {
                        if (idx !== -1)
                            flags.splice(idx, 1)
                    }

                    root.backendValue.expression = flags.join(" | ")
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
