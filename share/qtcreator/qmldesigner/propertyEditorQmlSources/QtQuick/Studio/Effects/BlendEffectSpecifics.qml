// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Blend")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Mode")
                tooltip: qsTr("The mode which is used when foreground source is blended over "
                              + "source.")
            }

            SecondColumnLayout {
                ComboBox {
                    id: blendMode
                    backendValue: backendValues.mode
                    useInteger: true
                    manualMapping: true
                    model: ["normal", "addition", "average", "color", "colorBurn", "colorDodge",
                        "darken", "darkerColor", "difference", "divide", "exclusion", "hardLight",
                        "hue", "lighten", "lighterColor", "lightness", "multiply", "negation",
                        "saturation", "screen", "subtract", "softLight"]
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth

                    property bool block: false

                    onValueFromBackendChanged: blendMode.fromBackendToFrontend()

                    onCurrentTextChanged: {
                        if (!__isCompleted)
                            return

                        if (block)
                            return

                        backendValues.mode.value = blendMode.model[blendMode.currentIndex]
                    }

                    Connections {
                        target: modelNodeBackend
                        onSelectionChanged: blendMode.fromBackendToFrontend()
                    }

                    function fromBackendToFrontend() {
                        if (!__isCompleted)
                            return

                        block = true

                        currentIndex = blendMode.model.indexOf(backendValues.mode.value)

                        block = false
                    }
                }

                ExpandingSpacer {}
            }

            PropertyLabel {
                text: qsTr("Foreground source")
                tooltip: qsTr("The component that is going to be blended over the source.")
            }

            SecondColumnLayout {
                ItemFilterComboBox {
                    typeFilter: "QtQuick.Item"
                    validator: RegExpValidator { regExp: /(^$|^[a-z_]\w*)/ }
                    backendValue: backendValues.foregroundSource
                    implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Caching")

        SectionLayout {
            PropertyLabel {
                text: qsTr("Cached")
                tooltip: qsTr("Caches the effect output pixels to improve the rendering "
                              + "performance.")
            }

            SecondColumnLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.cached
                    text: backendValues.cached.valueToString
                    implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                   + StudioTheme.Values.actionIndicatorWidth
                }

                ExpandingSpacer {}
            }
        }
    }
}
