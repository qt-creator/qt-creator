// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import HelperWidgets
import StudioTheme as StudioTheme

Section {
    id: root
    caption: qsTr("Component")

    anchors.left: parent.left
    anchors.right: parent.right

    property bool showState: false

    SectionLayout {
        PropertyLabel {
            text: qsTr("Type")
            tooltip: qsTr("Sets the QML type of the component.")
        }

        SecondColumnLayout {
            z: 2

            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            RoundedPanel {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                height: StudioTheme.Values.height

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: StudioTheme.Values.inputHorizontalPadding
                    anchors.topMargin: StudioTheme.Values.typeLabelVerticalShift
                    text: backendValues.__classNamePrivateInternal.value
                }

                ToolTipArea {
                    anchors.fill: parent
                    onDoubleClicked: {
                        typeLineEdit.text = backendValues.__classNamePrivateInternal.value
                        typeLineEdit.visible = !typeLineEdit.visible
                        typeLineEdit.forceActiveFocus()
                    }
                    tooltip: qsTr("Sets the QML type of the component.")
                    enabled: !modelNodeBackend.multiSelection
                }

                ExpressionTextField {
                    id: typeLineEdit
                    z: 2
                    completeOnlyTypes: true
                    replaceCurrentTextByCompletion: true
                    anchors.fill: parent
                    visible: false
                    fixedSize: true

                    property bool blockEditingFinished: false

                    onEditingFinished: {
                        if (typeLineEdit.blockEditingFinished)
                            return

                        typeLineEdit.blockEditingFinished = true

                        if (typeLineEdit.visible)
                            changeTypeName(typeLineEdit.text.trim())

                        typeLineEdit.visible = false
                        typeLineEdit.blockEditingFinished = false
                        typeLineEdit.completionList.model = null
                    }

                    onRejected: {
                        typeLineEdit.visible = false
                        typeLineEdit.completionList.model = null
                    }
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("ID")
            tooltip: qsTr("Sets a unique identification or name.")
        }

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            LineEdit {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: StudioTheme.Values.singleControlColumnWidth
                backendValue: backendValues.id
                placeholderText: qsTr("id")
                text: backendValues.id.value
                showTranslateCheckBox: false
                showExtendedFunctionButton: false
                enabled: !modelNodeBackend.multiSelection
            }

            Spacer { implicitWidth: StudioTheme.Values.twoControlColumnGap }

            Rectangle {
                id: aliasIndicator
                color: "transparent"
                border.color: "transparent"
                implicitWidth: StudioTheme.Values.iconAreaWidth // TODO dedicated value
                implicitHeight: StudioTheme.Values.height // TODO dedicated value
                z: 10

                T.Label {
                    id: aliasIndicatorIcon
                    enabled: !modelNodeBackend.multiSelection && anchorBackend.hasParent
                    anchors.fill: parent
                    text: StudioTheme.Constants.alias
                    color: StudioTheme.Values.themeTextColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.myIconFontSize + 4 // TODO
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    states: [
                        State {
                            name: "default"
                            when: !toolTipArea.containsMouse && !hasAliasExport
                            PropertyChanges {
                                target: aliasIndicatorIcon
                                color: StudioTheme.Values.themeLinkIndicatorColor
                            }
                        },
                        State {
                            name: "hover"
                            when: toolTipArea.containsMouse && !hasAliasExport
                            PropertyChanges {
                                target: aliasIndicatorIcon
                                color: StudioTheme.Values.themeLinkIndicatorColorHover
                            }
                        },
                        State {
                            name: "active"
                            when: hasAliasExport
                            PropertyChanges {
                                target: aliasIndicatorIcon
                                color: StudioTheme.Values.themeAliasIconChecked
                            }
                        },
                        State {
                            name: "disable"
                            when: !aliasIndicator.enabled
                            PropertyChanges {
                                target: aliasIndicatorIcon
                                color: StudioTheme.Values.themeLinkIndicatorColorDisabled
                            }
                        }
                    ]
                }

                ToolTipArea {
                    id: toolTipArea
                    enabled: !modelNodeBackend.multiSelection && anchorBackend.hasParent
                    anchors.fill: parent
                    onClicked: toogleExportAlias()
                    tooltip: qsTr("Exports this component as an alias property of the root component.")
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            visible: root.showState
            text: qsTr("State")
            tooltip: qsTr("Sets the state of the component.")
        }

        SecondColumnLayout {
            visible: root.showState

            ComboBox {
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                width: implicitWidth
                editable: true
                backendValue: backendValues.state
                model: allStateNames
                valueType: ComboBox.String
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: annotationEditor.hasAuxData ? qsTr("Annotation") : ""
            tooltip: qsTr("Adds a note with a title to explain the component.")
        }

        SecondColumnLayout {
            enabled: !modelNodeBackend.multiSelection

            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            Row {
                spacing: -StudioTheme.Values.border
                visible: annotationEditor.hasAuxData

                LineEdit {
                    id: annotationEdit
                    width: StudioTheme.Values.singleControlColumnWidth
                            - editAnnotationButton.width
                            - removeAnnotationButton.width
                            + 2 * StudioTheme.Values.border
                    backendValue: backendValues.customId__AUX
                    placeholderText: qsTr("Descriptive text")
                    text: backendValue.value
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false
                    onHoveredChanged: annotationEditor.checkAux()
                    onActiveFocusChanged: annotationEdit.z = activeFocus ? 10 : 0
                }

                AbstractButton {
                    id: editAnnotationButton
                    buttonIcon: StudioTheme.Constants.edit
                    tooltip: qsTr("Edit Annotation")
                    onClicked: annotationEditor.showWidget()
                    onHoveredChanged: annotationEditor.checkAux()
                }

                AbstractButton {
                    id: removeAnnotationButton
                    buttonIcon: StudioTheme.Constants.closeCross
                    tooltip: qsTr("Remove Annotation")
                    onClicked: annotationEditor.removeFullAnnotation()
                    onHoveredChanged: annotationEditor.checkAux()
                }
            }

            AbstractButton {
                id: addAnnotationButton
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: StudioTheme.Values.singleControlColumnWidth
                visible: !annotationEditor.hasAuxData
                buttonIcon: qsTr("Add Annotation")
                iconFont: StudioTheme.Constants.font
                tooltip: qsTr("Adds a note with a title to explain the component.")
                onClicked: annotationEditor.showWidget()
                onHoveredChanged: annotationEditor.checkAux()
            }

            ExpandingSpacer {}

            AnnotationEditor {
                id: annotationEditor

                modelNodeBackendProperty: modelNodeBackend

                property bool hasAuxData: (annotationEditor.hasAnnotation || annotationEditor.hasCustomId)

                onModelNodeBackendChanged: checkAux()
                onCustomIdChanged: checkAux()
                onAnnotationChanged: checkAux()

                function checkAux() {
                    hasAuxData = (annotationEditor.hasAnnotation || annotationEditor.hasCustomId)
                    annotationEdit.update()
                }

                onAccepted: hideWidget()
                onCanceled: hideWidget()
            }
        }
    }
}
