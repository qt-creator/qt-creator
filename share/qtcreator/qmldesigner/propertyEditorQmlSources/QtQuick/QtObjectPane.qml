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

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    ScrollView {
        anchors.fill: parent

        Column {
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
                            width: 240
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false
                            enabled: !modelNodeBackend.multiSelection
                        }

                        Image {
                            visible: !modelNodeBackend.multiSelection
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            source: hasAliasExport ? "image://icons/alias-export-checked" : "image://icons/alias-export-unchecked"
                            ToolTipArea {
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
                    id: tab
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

                            property int loaderHeight: specificsTwo.item.height + tabView.extraHeight
                            onLoaderHeightChanged: tabView.specficsTwoHeight = loaderHeight

                            onLoaded: {
                                tabView.specficsTwoHeight = loaderHeight
                            }
                        }

                        Loader {
                            anchors.left: parent.left
                            anchors.right: parent.right

                            id: specificsOne;
                            source: specificsUrl;

                            property int loaderHeight: specificsOne.item.height + tabView.extraHeight
                            onLoaderHeightChanged: tabView.specficsHeight = loaderHeight

                            onLoaded: {
                                tabView.specficsOneHeight = loaderHeight
                            }
                        }
                    }
                }
            }
        }
    }
}
