// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.SplitView {
    id: root

    property HelperWidgets.QmlMaterialNodeProxy backend: materialNodeBackend
    property alias showImage: previewLoader.active
    property Component previewComponent: null

    width: parent.width
    implicitHeight: showImage ? previewLoader.activeHeight + nameSection.implicitHeight : nameSection.implicitHeight

    orientation: Qt.Vertical

    Loader {
        id: previewLoader

        property real activeHeight: previewLoader.active ? implicitHeight : 0

        SplitView.fillWidth: true
        SplitView.minimumWidth: 152
        SplitView.preferredHeight: previewLoader.visible ? Math.min(root.width * 0.75, 400) : 0
        SplitView.minimumHeight: previewLoader.visible ? 150 : 0
        SplitView.maximumHeight: previewLoader.visible ? 600 : 0

        visible: previewLoader.active && previewLoader.item

        sourceComponent: root.previewComponent
    }

    HelperWidgets.Section {
        id: nameSection

        // Section with hidden header is used so properties are aligned with the other sections' properties
        hideHeader: true
        SplitView.fillWidth: true
        SplitView.preferredHeight: implicitHeight
        SplitView.maximumHeight: implicitHeight
        bottomPadding: StudioTheme.Values.sectionPadding * 2
        collapsible: false

        HelperWidgets.SectionLayout {
            HelperWidgets.PropertyLabel { text: qsTr("Name") }

            HelperWidgets.SecondColumnLayout {
                HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                HelperWidgets.LineEdit {
                    id: matName

                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    width: StudioTheme.Values.singleControlColumnWidth
                    placeholderText: qsTr("Material name")
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false
                    enabled: !hasMultiSelection

                    Timer {
                        running: true
                        interval: 0
                        onTriggered: matName.backendValue = backendValues.objectName
                        // backendValues.objectName is not available yet without the Timer
                    }

                    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                    validator: RegularExpressionValidator { regularExpression: /^(\w+\s)*\w+$/ }
                }

                HelperWidgets.ExpandingSpacer {}
            }

            HelperWidgets.PropertyLabel { text: qsTr("Type") }

            HelperWidgets.SecondColumnLayout {
                HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                HelperWidgets.ComboBox {
                    id : typeComboBox

                    currentIndex: backend.possibleTypeIndex
                    model: backend.possibleTypes
                    showExtendedFunctionButton: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    enabled: backend.possibleTypes.length > 1

                    onActivated: changeTypeName(currentValue)

                    Binding {
                        when: !typeComboBox.open
                        typeComboBox.currentIndex: backend.possibleTypeIndex
                    }
                }
            }
        }
    }
}
