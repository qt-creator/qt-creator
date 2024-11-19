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

    icons: HelperWidgets.IconButton {
        id: codeButton

        icon: StudioTheme.Constants.codeEditor_medium
        transparentBg: true
        buttonSize: 21
        iconSize: StudioTheme.Values.smallIconFontSize
        iconColor: root.codeEditorOpen
                   ? StudioTheme.Values.themeInteraction
                   : StudioTheme.Values.themeTextColor
        iconScale: codeButton.containsMouse ? 1.2 : 1
        implicitWidth: width
        tooltip: qsTr("Open code editor")
        onClicked: root.backendModel.openCodeEditor(index)
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

    Column {
        spacing: 10

        Repeater {
            id: repeater
            model: nodeUniformsModel

            EffectCompositionNodeUniform {
                id: effectCompositionNodeUniform
                width: root.width - StudioTheme.Values.scrollBarThicknessHover
                editing: root.editedUniformIndex === index
                disableMoreMenu: root.editedUniformIndex >= 0

                onReset: nodeUniformsModel.resetData(index)
                onRemove: {
                    if (root.backendModel.isNodeUniformInUse(root.modelIndex, index)) {
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
            }
        }
    }

    Item {
        id: addProperty
        width: root.width - StudioTheme.Values.scrollBarThicknessHover
        height: addPropertyForm.visible && addPropertyForm.parent === addProperty
                ? addPropertyForm.height : 50

        HelperWidgets.Button {
            id: addPropertyButton
            width: 130
            height: 35
            text: qsTr("Add Property")
            visible: !addPropertyForm.visible
            anchors.horizontalCenter: parent.horizontalCenter
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
}

