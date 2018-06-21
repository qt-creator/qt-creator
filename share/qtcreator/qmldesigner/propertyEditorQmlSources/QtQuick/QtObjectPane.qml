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
                caption: qsTr("Type")

                anchors.left: parent.left
                anchors.right: parent.right

                SectionLayout {
                    Label {
                        text: qsTr("Type")

                    }

                    SecondColumnLayout {

                        Label {
                            text: backendValues.className.value
                            width: lineEdit.width
                        }
                    }

                    Label {
                        text: qsTr("id")
                    }

                    SecondColumnLayout {
                        LineEdit {
                            id: lineEdit
                            enabled: isBaseState
                            backendValue: backendValues.id
                            placeholderText: qsTr("id")
                            text: backendValues.id.value
                            Layout.fillWidth: true
                            showTranslateCheckBox: false
                            showExtendedFunctionButton: false
                        }
                        // workaround: without this item the lineedit does not shrink to the
                        // right size after resizing to a wider width
                        Item {
                            width: 0
                            height: 1
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
                        }
                    }
                }
            }
        }
    }
}
