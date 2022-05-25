/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

Column {
    id: root

    signal toolBarAction(int action)

    function refreshPreview()
    {
        materialPreview.source = ""
        materialPreview.source = "image://materialEditor/preview"
    }

    anchors.left: parent.left
    anchors.right: parent.right

    MaterialEditorToolBar {
        width: root.width

        onToolBarAction: (action) => root.toolBarAction(action)
    }

    Item { width: 1; height: 10 } // spacer

    Rectangle {
        width: 152
        height: 152
        color: "#000000"
        anchors.horizontalCenter: parent.horizontalCenter

        Image {
            id: materialPreview
            width: 150
            height: 150
            anchors.centerIn: parent
            source: "image://materialEditor/preview"
            cache: false
        }
    }

    Section {
        // Section with hidden header is used so properties are aligned with the other sections' properties
        hideHeader: true
        width: parent.width

        SectionLayout {
            PropertyLabel { text: qsTr("Name") }

            SecondColumnLayout {
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                LineEdit {
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    width: StudioTheme.Values.singleControlColumnWidth
                    backendValue: backendValues.objectName
                    placeholderText: qsTr("Material name")

                    text: backendValues.id.value
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false

                    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                    validator: RegExpValidator { regExp: /^(\w+\s)*\w+$/ }
                }

                ExpandingSpacer {}
            }

            PropertyLabel { text: qsTr("Type") }

            SecondColumnLayout {
                Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                ComboBox {
                    currentIndex: {
                        if (backendValues.__classNamePrivateInternal.value === "CustomMaterial")
                            return 2

                        if (backendValues.__classNamePrivateInternal.value === "PrincipledMaterial")
                            return 1

                        return 0
                    }

                    model: ["DefaultMaterial", "PrincipledMaterial", "CustomMaterial"]
                    showExtendedFunctionButton: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth

                    onActivated: changeTypeName(currentValue)
                }
            }
        }
    }
}
