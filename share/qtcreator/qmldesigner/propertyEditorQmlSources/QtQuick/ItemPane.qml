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

Rectangle {
    id: itemPane
    width: 320
    height: 400
    color: Theme.qmlDesignerBackgroundColorDarkAlternate()

    ScrollView {
        anchors.fill: parent

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
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
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
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false
                        }
                        // workaround: without this item the lineedit does not shrink to the
                        // right size after resizing to a wider width

                        Image {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            source: hasAliasExport ? "image://icons/alias-export-checked" : "image://icons/alias-export-unchecked"
                            ToolTipArea {
                                anchors.fill: parent
                                onClicked: toogleExportAlias()
                                tooltip: qsTr("Toggles whether this item is exported as an alias property of the root item.")
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
                height: Math.max(layoutSectionHeight, specficsHeight)

                property int layoutSectionHeight: 400
                property int specficsOneHeight: 0
                property int specficsTwoHeight: 0

                property int specficsHeight: Math.max(specficsOneHeight, specficsTwoHeight)

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

                            onLoaded: {
                                tabView.specficsTwoHeight = specificsTwo.item.height + tabView.extraHeight
                            }
                        }

                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right

                            id: specificsOne;
                            source: specificsUrl;

                            onLoaded: {
                                tabView.specficsOneHeight = specificsOne.item.height + tabView.extraHeight
                            }
                        }
                    }
                }

                Tab {
                    title: qsTr("Layout")
                    component: Column {
                        anchors.left: parent.left
                        anchors.right: parent.right

                        LayoutSection {
                            property int childRectHeight: childrenRect.height
                            onChildRectHeightChanged: {
                                tabView.layoutSectionHeight = childRectHeight + tabView.extraHeight
                            }
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
                    }
                }
            }
        }
    }
}
