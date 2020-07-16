/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

Rectangle {
    id: itemPane
    width: 320
    height: 400
    color: Theme.qmlDesignerBackgroundColorDarkAlternate()

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    ScrollView {
        anchors.fill: parent

        Column {
            id: rootColumn
            y: -1
            width: itemPane.width
            Section {
                caption: qsTr("Type")

                anchors.left: parent.left
                anchors.right: parent.right

                SectionLayout {
                    Label {
                        text: qsTr("Type")
                    }

                    SecondColumnLayout {
                        z: 2

                        RoundedPanel {
                            Layout.fillWidth: true
                            height: StudioTheme.Values.height

                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: StudioTheme.Values.inputHorizontalPadding
                                text: backendValues.className.value
                            }
                            ToolTipArea {
                                anchors.fill: parent
                                onDoubleClicked: {
                                    typeLineEdit.text = backendValues.className.value
                                    typeLineEdit.visible = ! typeLineEdit.visible
                                    typeLineEdit.forceActiveFocus()
                                }
                                tooltip: qsTr("Change the type of this item.")
                                enabled: !modelNodeBackend.multiSelection
                            }

                            ExpressionTextField {
                                z: 2
                                id: typeLineEdit
                                completeOnlyTypes: true
                                replaceCurrentTextByCompletion: true
                                anchors.fill: parent

                                visible: false

                                showButtons: false
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
                        Item {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                        }
                    }

                    Label {
                        text: qsTr("id")
                    }

                    SecondColumnLayout {
                        spacing: 2
                        LineEdit {
                            id: lineEdit

                            backendValue: backendValues.id
                            placeholderText: qsTr("id")
                            text: backendValues.id.value
                            Layout.fillWidth: true
                            width: 240
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false
                            enabled: !modelNodeBackend.multiSelection
                        }

                        Rectangle {
                            id: aliasIndicator
                            color: "transparent"
                            border.color: "transparent"
                            implicitWidth: StudioTheme.Values.height
                            implicitHeight: StudioTheme.Values.height
                            z: 10

                            Label {
                                id: aliasIndicatorIcon
                                enabled: !modelNodeBackend.multiSelection
                                anchors.fill: parent
                                text: {
                                    if (!aliasIndicatorIcon.enabled)
                                        return StudioTheme.Constants.idAliasOff

                                    return hasAliasExport ? StudioTheme.Constants.idAliasOn : StudioTheme.Constants.idAliasOff
                                }
                                color: {
                                    if (!aliasIndicatorIcon.enabled)
                                        return StudioTheme.Values.themeTextColorDisabled

                                    return hasAliasExport ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeTextColor
                                }
                                font.family: StudioTheme.Constants.iconFont.family
                                font.pixelSize: Math.round(16 * StudioTheme.Values.scaleFactor)
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                states: [
                                    State {
                                        name: "hovered"
                                        when: toolTipArea.containsMouse && aliasIndicatorIcon.enabled
                                        PropertyChanges {
                                            target: aliasIndicatorIcon
                                            scale: 1.2
                                        }
                                    }
                                ]
                            }

                            ToolTipArea {
                                id: toolTipArea
                                enabled: !modelNodeBackend.multiSelection
                                anchors.fill: parent
                                onClicked: toogleExportAlias()
                                tooltip: qsTr("Toggles whether this item is exported as an alias property of the root item.")
                            }
                        }
                    }
                }
            }

            Item {
                height: 4
                width: 4
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                Loader {
                    id: specificsTwo
                    anchors.left: parent.left
                    anchors.right: parent.right
                    visible: theSource !== ""
                    sourceComponent: specificQmlComponent

                    property string theSource: specificQmlData

                    onTheSourceChanged: {
                        active = false
                        active = true
                    }
                }

                Loader {
                    id: specificsOne
                    anchors.left: parent.left
                    anchors.right: parent.right
                    source: specificsUrl
                }
            }
        }
    }
}
