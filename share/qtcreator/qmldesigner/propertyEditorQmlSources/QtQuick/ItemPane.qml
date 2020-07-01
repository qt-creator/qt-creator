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

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls
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
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

        Column {
            y: -1
            width: itemPane.width
            Section {
                z: 2
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
                            height: 24

                            Label {
                                x: 6
                                anchors.fill: parent
                                anchors.leftMargin: 16

                                text: backendValues.className.value
                                verticalAlignment: Text.AlignVCenter
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

                                anchors.fill: parent

                                visible: false

                                showButtons: false
                                fixedSize: true

                                onEditingFinished: {
                                    if (visible)
                                        changeTypeName(typeLineEdit.text.trim())
                                    visible = false
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
                        // workaround: without this item the lineedit does not shrink to the
                        // right size after resizing to a wider width

                        Image {
                            visible: !modelNodeBackend.multiSelection
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            horizontalAlignment: Image.AlignHCenter
                            verticalAlignment: Image.AlignVCenter
                            source: hasAliasExport ? "image://icons/alias-export-checked" : "image://icons/alias-export-unchecked"
                            ToolTipArea {
                                enabled: !modelNodeBackend.multiSelection
                                anchors.fill: parent
                                onClicked: toogleExportAlias()
                                tooltip: qsTr("Toggles whether this item is exported as an alias property of the root item.")
                            }
                        }
                        Item { //dummy object to preserve layout in case of multiselection
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            enabled: modelNodeBackend.multiSelection
                            visible: enabled
                        }
                    }

                    Label {
                        text: qsTr("Custom id")
                    }

                    SecondColumnLayout {
                        enabled: !modelNodeBackend.multiSelection
                        spacing: 2

                        LineEdit {
                            id: annotationEdit
                            visible: annotationEditor.hasAuxData

                            backendValue: backendValues.customId__AUX
                            placeholderText: qsTr("customId")
                            text: backendValue.value
                            Layout.fillWidth: true
                            Layout.preferredWidth: 240
                            width: 240
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false

                            onHoveredChanged: annotationEditor.checkAux()
                        }

                        StudioControls.AbstractButton {
                            id: editAnnotationButton
                            visible: annotationEditor.hasAuxData

                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            width: 22

                            buttonIcon: StudioTheme.Constants.edit

                            onClicked: annotationEditor.showWidget()
                            onHoveredChanged: annotationEditor.checkAux()
                        }

                        StudioControls.AbstractButton {
                            id: removeAnnotationButton
                            visible: annotationEditor.hasAuxData

                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            width: 22

                            buttonIcon: StudioTheme.Constants.closeCross

                            onClicked: annotationEditor.removeFullAnnotation()
                            onHoveredChanged: annotationEditor.checkAux()
                        }

                        StudioControls.AbstractButton {
                            id: addAnnotationButton
                            visible: !annotationEditor.hasAuxData

                            buttonIcon: qsTr("Add Annotation")
                            iconFont: StudioTheme.Constants.font
                            Layout.fillWidth: true
                            Layout.preferredWidth: 240
                            width: 240

                            onClicked: annotationEditor.showWidget()

                            onHoveredChanged: annotationEditor.checkAux()
                        }

                        Item {
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            visible: !annotationEditor.hasAuxData
                        }

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

                            onAccepted: {
                                hideWidget()
                            }

                            onCanceled: {
                                hideWidget()
                            }
                        }
                    }
                }
            }

            GeometrySection {
            }

            Section {
                anchors.left: parent.left
                anchors.right: parent.right

                caption: qsTr("Visibility")

                SectionLayout {
                    rows: 2
                    Label {
                        text: qsTr("Visibility")
                    }

                    SecondColumnLayout {

                        CheckBox {
                            text: qsTr("Is Visible")
                            backendValue: backendValues.visible
                        }

                        Item {
                            width: 10
                            height: 10

                        }

                        CheckBox {
                            text: qsTr("Clip")
                            backendValue: backendValues.clip
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: qsTr("Opacity")
                    }

                    SecondColumnLayout {
                        SpinBox {
                            sliderIndicatorVisible: true
                            backendValue: backendValues.opacity
                            decimals: 2

                            minimumValue: 0
                            maximumValue: 1
                            hasSlider: true
                            stepSize: 0.1
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Item {
                height: 4
                width: 4
            }

            TabView {
                anchors.left: parent.left
                anchors.right: parent.right
                frameVisible: false

                id: tabView

                height: currentHeight + extraHeight

                property int currentHeight: getTab(currentIndex).item.implicitHeight
                property int extraHeight: 40

                Tab {
                    title: backendValues.className.value

                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            visible: theSource !== ""

                            id: specificsTwo;
                            sourceComponent: specificQmlComponent

                            property string theSource: specificQmlData

                            onTheSourceChanged: {
                                active = false
                                active = true
                            }
                        }

                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right

                            id: specificsOne;
                            source: specificsUrl;

                            property int loaderHeight: specificsOne.item.height + tabView.extraHeight
                        }
                    }
                }

                Tab {
                    title: qsTr("Layout")
                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LayoutSection {
                        }

                        MarginSection {
                            visible: anchorBackend.isInLayout
                            backendValueTopMargin: backendValues.Layout_topMargin
                            backendValueBottomMargin: backendValues.Layout_bottomMargin
                            backendValueLeftMargin: backendValues.Layout_leftMargin
                            backendValueRightMargin: backendValues.Layout_rightMargin
                            backendValueMargins: backendValues.Layout_margins
                        }

                        AlignDistributeSection {
                            visible: !anchorBackend.isInLayout
                        }
                    }
                }

                Tab {
                    anchors.fill: parent
                    title: qsTr("Advanced")
                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        AdvancedSection {
                        }
                        LayerSection {
                        }
                    }
                }
            }
        }
    }
}
