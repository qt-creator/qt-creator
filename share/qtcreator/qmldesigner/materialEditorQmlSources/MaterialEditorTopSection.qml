// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme

StudioControls.SplitView {
    id: root

    property alias showImage: previewLoader.active
    property Component previewComponent: null

    width: parent.width
    implicitHeight: showImage ? previewLoader.implicitHeight + nameSection.implicitHeight : nameSection.implicitHeight

    orientation: Qt.Vertical

    Loader {
        id: previewLoader

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
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    width: StudioTheme.Values.singleControlColumnWidth
                    backendValue: backendValues.objectName
                    placeholderText: qsTr("Material name")

                    text: backendValues.id.value
                    showTranslateCheckBox: false
                    showExtendedFunctionButton: false

                    // allow only alphanumeric characters, underscores, no space at start, and 1 space between words
                    validator: RegularExpressionValidator { regularExpression: /^(\w+\s)*\w+$/ }
                }

                HelperWidgets.ExpandingSpacer {}
            }

            HelperWidgets.PropertyLabel { text: qsTr("Type") }

            HelperWidgets.SecondColumnLayout {
                HelperWidgets.Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

                HelperWidgets.ComboBox {
                    currentIndex: possibleTypeIndex
                    model: possibleTypes
                    showExtendedFunctionButton: false
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                    enabled: possibleTypes.length > 1

                    onActivated: changeTypeName(currentValue)
                }
            }
        }
    }
}
