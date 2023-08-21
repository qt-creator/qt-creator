// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Item {
    id: root

    height: 22

    RowLayout {
        spacing: 10
        anchors.fill: parent

        Text {
            text: uniformName
            color: StudioTheme.Values.themeTextColor
            font.pointSize: StudioTheme.Values.smallFontSize
            horizontalAlignment: Text.AlignRight
            Layout.maximumWidth: 140
            Layout.minimumWidth: 140
            Layout.preferredWidth: 140
        }

        Loader {
            sourceComponent: floatValue // TODO: set component based on prop type
            Layout.fillWidth: true
        }
    }

    Component {
        id: floatValue

        Row {
            width: parent.width
            spacing: 5

            StudioControls.RealSpinBox {
                id: spinBox

                width: 40
                actionIndicatorVisible: false
                spinBoxIndicatorVisible: false
                inputHAlignment: Qt.AlignHCenter
                realFrom: uniformMinValue
                realTo: uniformMaxValue
                realValue: uniformValue
                realStepSize: .01
                decimals: 2
                onRealValueModified: uniformValue = realValue
            }

            StudioControls.Slider {
                id: slider

                width: parent.width - 80
                labels: false
                decimals: 2
                actionIndicatorVisible: false
                from: uniformMinValue
                to: uniformMaxValue
                value: uniformValue
                onMoved: {
                    uniformValue = value
                    spinBox.realValue = value // binding isn't working for this property so update it
                }
            }
        }
    }

    Component { // TODO
        id: colorValue

        Row {
            width: parent.width
            spacing: 5

            HelperWidgets.ColorEditor {
                backendValue: "#ffff00"
            }
        }
    }
}
