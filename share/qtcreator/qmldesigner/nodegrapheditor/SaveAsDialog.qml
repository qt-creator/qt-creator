// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

StudioControls.Dialog {
    id: root

    property alias fileName: nameText.text

    signal save(string fileName)

    closePolicy: Popup.CloseOnEscape
    implicitHeight: 160
    implicitWidth: 350
    modal: true
    title: qsTr("Save node graph")

    contentItem: Item {
        Column {
            spacing: 2

            Row {
                id: row

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Node graph name: ")
                }

                StudioControls.TextField {
                    id: nameText

                    actionIndicator.visible: false
                    translationIndicator.visible: false

                    Keys.onEnterPressed: btnSave.onClicked()
                    Keys.onEscapePressed: root.reject()
                    Keys.onReturnPressed: btnSave.onClicked()
                    onTextChanged: {
                        let errMsg = "";
                        if (/[^A-Za-z0-9_]+/.test(text))
                            errMsg = qsTr("Name contains invalid characters.");
                        // if (!/^[A-Z]/.test(text))
                        //     errMsg = qsTr("Name must start with a capital letter.")
                        if (text.length < 1)
                            errMsg = qsTr("Name must have at least 1 character.");
                        else if (/\s/.test(text))
                            errMsg = qsTr("Name cannot contain white space.");

                        emptyText.text = errMsg;
                        btnSave.enabled = errMsg.length === 0;
                    }
                }
            }

            Text {
                id: emptyText

                anchors.right: row.right
                color: StudioTheme.Values.themeError
            }
        }

        Row {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            spacing: 2

            HelperWidgets.Button {
                id: btnSave

                enabled: nameText.text !== ""
                text: qsTr("Save")

                onClicked: {
                    if (!enabled) // needed since this event handler can be triggered from keyboard events
                        return;
                    root.save(nameText.text);
                    root.accept(); // TODO: confirm before overriding node graph with same name
                }
            }

            HelperWidgets.Button {
                text: qsTr("Cancel")

                onClicked: {
                    root.reject();
                }
            }
        }
    }

    onOpened: {
        nameText.text = "";
        nameText.selectAll();
        nameText.forceActiveFocus();
        emptyText.text = "";
    }
}
