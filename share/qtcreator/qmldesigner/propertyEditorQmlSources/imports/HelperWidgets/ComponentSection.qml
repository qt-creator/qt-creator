/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import QtQuick.Templates 2.15 as T
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Section {
    id: root
    caption: qsTr("Component")

    anchors.left: parent.left
    anchors.right: parent.right

    property bool showState: false

    SectionLayout {
        PropertyLabel { text: qsTr("Type") }

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
                    text: backendValues.className.value
                }

                ToolTipArea {
                    anchors.fill: parent
                    onDoubleClicked: {
                        typeLineEdit.text = backendValues.className.value
                        typeLineEdit.visible = !typeLineEdit.visible
                        typeLineEdit.forceActiveFocus()
                    }
                    tooltip: qsTr("Changes the type of this component.")
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

        PropertyLabel { text: qsTr("ID") }

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

        PropertyLabel { text: qsTr("Name") }

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
                    tooltip: qsTr("Edit annotation")
                    onClicked: annotationEditor.showWidget()
                    onHoveredChanged: annotationEditor.checkAux()
                }

                AbstractButton {
                    id: removeAnnotationButton
                    buttonIcon: StudioTheme.Constants.closeCross
                    tooltip: qsTr("Remove annotation")
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

        PropertyLabel {
            visible: root.showState
            text: qsTr("State")
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
    }
}
