// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

HelperWidgets.Section {
    id: root

    readonly property var backendModel: EffectComposerBackend.effectComposerModel
    readonly property bool codeEditorOpen: root.backendModel.codeEditorIndex === root.modelIndex

    property int modelIndex: 0
    property int editedUniformIndex: -1

    signal ensureVisible(visibleY: real)

    function emitEnsure(item)
    {
        if (item.visible) {
            let ensureY = item.mapToItem(root, Qt.point(0, 0)).y + item.height + root.y + 30
            root.ensureVisible(ensureY)
        }
    }

    caption: nodeName
    category: "EffectComposer"

    draggable: !isDependency
    fillBackground: true
    showCloseButton: !isDependency
    closeButtonToolTip: qsTr("Remove")
    closeButtonIcon: StudioTheme.Constants.deletepermanently_medium
    visible: repeater.count > 0 || !isDependency || isCustom

    onCloseButtonClicked: {
        root.backendModel.removeNode(root.modelIndex)
    }

    showEyeButton: !isDependency
    eyeEnabled: nodeEnabled
    eyeButtonToolTip: qsTr("Enable/Disable Node")

    onEyeButtonClicked: {
        nodeEnabled = root.eyeEnabled
    }

    content: Label {
        text: root.caption
        color: root.labelColor
        elide: Text.ElideRight
        font.pixelSize: root.sectionFontSize
        font.capitalization: root.labelCapitalization
        anchors.verticalCenter: parent?.verticalCenter
        textFormat: Text.RichText
        leftPadding: StudioTheme.Values.toolbarSpacing
    }

    TextEdit {
        id: warningText

        visible: false
        height: 60
        width: root.width
        text: qsTr("A node with this name already exists.\nSuffix was added to make the name unique.")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: StudioTheme.Values.themeWarning
        readOnly: true

        onVisibleChanged: {
            if (warningText.visible) {
                warningTimer.running = true
                root.expanded = true // so that warning is visible
            }
        }

        Timer {
            id: warningTimer

            interval: 12000
            repeat: false
            running: false

            onTriggered: {
                warningText.visible = false
                warningTimer.running = false
            }
        }
    }

    ConfirmPropertyRemoveForm {
        id: confirmRemoveForm

        property int uniformIndex: -1

        width: root.width - StudioTheme.Values.scrollBarThicknessHover - 8
        visible: false

        onHeightChanged: root.emitEnsure(confirmRemoveForm)
        onVisibleChanged: root.emitEnsure(confirmRemoveForm)

        onAccepted: {
            confirmRemoveForm.parent = root
            nodeUniformsModel.remove(confirmRemoveForm.uniformIndex)
        }

        onCanceled: confirmRemoveForm.parent = root
    }

    MouseArea {
        id: nameEditMouseArea

        parent: root.content // reparent to section header item for easy positioning
        x: -2
        y: -2
        height: parent.height + 4
        width: parent.width + nameEditButton.width + 4
        hoverEnabled: true
        acceptedButtons: Qt.NoButton

        Rectangle {
            id: nameEditField

            width: root.width - parent.parent.parent.x - 30
            height: parent.height

            visible: false
            border.width: 1
            border.color: StudioTheme.Values.themeControlOutline
            color: StudioTheme.Values.themeSectionHeadBackground

            StudioControls.TextField {
                id: nameEditText

                property bool initializing: false

                anchors.fill: parent
                anchors.margins: 2

                actionIndicatorVisible: false
                translationIndicatorVisible: false

                font.capitalization: Font.MixedCase

                onEditingFinished: {
                    nameEditField.visible = false
                    warningText.visible = !root.backendModel.changeNodeName(modelIndex, nameEditText.text)
                }

                onActiveFocusChanged: {
                    if (nameEditText.initializing)
                        nameEditText.initializing = false
                    else if (!nameEditText.activefocus)
                        nameEditField.visible = false
                }
            }
        }

        Rectangle {
            color: StudioTheme.Values.themeSectionHeadBackground
            width: nameEditButton.width
            height: nameEditButton.height
            x: Math.min(nameEditMouseArea.parent.width + 4, nameEditField.width - nameEditButton.width)
            anchors.verticalCenter: parent.verticalCenter
            visible: (nameEditMouseArea.containsMouse || nameEditButton.containsMouse)
                     && !nameEditField.visible && !isDependency

            HelperWidgets.IconButton {
                id: nameEditButton

                icon: StudioTheme.Constants.edit_small
                transparentBg: true
                buttonSize: 21
                iconSize: StudioTheme.Values.smallIconFontSize
                iconColor: StudioTheme.Values.themeTextColor
                iconScale: nameEditButton.containsMouse ? 1.2 : 1
                implicitWidth: width
                tooltip: qsTr("Edit effect node name")

                onPressed: (event) => {
                    nameEditText.text = nodeName
                    nameEditText.initializing = true
                    nameEditField.visible = true
                    nameEditText.forceActiveFocus()
                }
            }
        }
    }

    Row {
        width: parent.width
        height: uniformsCol.height
        spacing: 0

        Rectangle {
            width: 4
            height: parent.height
            color: codeEditorOpen ? StudioTheme.Values.themeInteraction : "transparent"
        }

        Column {
            id: uniformsCol

            width: parent.width - 4
            spacing: 0

            Repeater {
                id: repeater
                model: nodeUniformsModel

                EffectCompositionNodeUniform {
                    id: effectCompositionNodeUniform
                    width: root.width - StudioTheme.Values.scrollBarThicknessHover
                    editing: root.editedUniformIndex === index
                    disableMoreMenu: root.editedUniformIndex >= 0
                    isDependencyNode: isDependency
                    isCustomNode: isCustom

                    onReset: nodeUniformsModel.resetData(index)
                    onRemove: {
                        if (uniformIsInUse) {
                            confirmRemoveForm.parent = effectCompositionNodeUniform.editPropertyFormParent
                            confirmRemoveForm.uniformIndex = index
                            confirmRemoveForm.visible = true
                        } else {
                            nodeUniformsModel.remove(index)
                        }
                    }
                    onEdit: {
                        confirmRemoveForm.visible = false
                        confirmRemoveForm.parent = root
                        addPropertyForm.parent = effectCompositionNodeUniform.editPropertyFormParent
                        let dispNames = nodeUniformsModel.displayNames()
                        let filteredDispNames = dispNames.filter(name => name !== uniformDisplayName);
                        addPropertyForm.reservedDispNames = filteredDispNames
                        let uniNames = root.backendModel.uniformNames()
                        let filteredUniNames = uniNames.filter(name => name !== uniformName);
                        addPropertyForm.reservedUniNames = filteredUniNames
                        root.editedUniformIndex = index
                        addPropertyForm.showForEdit(uniformType, uniformControlType, uniformDisplayName,
                                                    uniformName, uniformDescription, uniformDefaultValue,
                                                    uniformMinValue, uniformMaxValue, uniformUserAdded)
                    }
                    onOpenCodeEditor: root.backendModel.openCodeEditor(root.modelIndex)
                }
            }

            Item {
                id: addProperty
                width: parent.width - StudioTheme.Values.scrollBarThicknessHover
                height: addPropertyForm.visible && addPropertyForm.parent === addProperty
                        ? addPropertyForm.height : 0
                visible: !isDependency

                AddPropertyForm {
                    id: addPropertyForm
                    visible: false
                    width: parent.width

                    effectNodeName: nodeName
                    onHeightChanged: root.emitEnsure(addPropertyForm)
                    onVisibleChanged: root.emitEnsure(addPropertyForm)

                    onAccepted: {
                        root.backendModel.addOrUpdateNodeUniform(root.modelIndex,
                                                                 addPropertyForm.propertyData(),
                                                                 root.editedUniformIndex)
                        addPropertyForm.parent = addProperty
                        root.editedUniformIndex = -1
                    }

                    onCanceled: {
                        addPropertyForm.parent = addProperty
                        root.editedUniformIndex = -1
                    }
                }
            }

            Row {
                height: 40
                visible: (root.backendModel.advancedMode || isCustom) && !isDependency && !addPropertyForm.visible
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 5

                HelperWidgets.Button {
                    width: 100
                    height: 30
                    text: qsTr("Add Property")
                    enabled: !addPropertyForm.visible
                    anchors.verticalCenter: parent.verticalCenter

                    onClicked: {
                        confirmRemoveForm.visible = false
                        confirmRemoveForm.parent = root
                        root.editedUniformIndex = -1
                        addPropertyForm.parent = addProperty
                        addPropertyForm.reservedDispNames = nodeUniformsModel.displayNames()
                        addPropertyForm.reservedUniNames = root.backendModel.uniformNames()
                        addPropertyForm.showForAdd()
                    }
                }

                HelperWidgets.Button {
                    width: 100
                    height: 30
                    text: qsTr("Show Code")
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.backendModel.openCodeEditor(index)
                }
            }
        }
    }
}

