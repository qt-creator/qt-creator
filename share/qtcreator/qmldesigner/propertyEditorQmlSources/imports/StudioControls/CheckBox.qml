// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

T.CheckBox {
    id: myCheckBox

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: myCheckBox.hovered && myCheckBox.enabled
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property alias labelVisible: checkBoxLabel.visible
    property alias labelColor: checkBoxLabel.color

    property alias fontFamily: checkBoxLabel.font.family
    property alias fontPixelSize: checkBoxLabel.font.pixelSize

    font.pixelSize: StudioTheme.Values.myFontSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: checkBoxLabel.visible ? StudioTheme.Values.checkBoxSpacing : 0
    hoverEnabled: true
    activeFocusOnTab: false

    ActionIndicator {
        id: actionIndicator
        myControl: myCheckBox
        width: actionIndicator.visible ? myCheckBox.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? myCheckBox.__actionIndicatorHeight : 0
    }

    indicator: Rectangle {
        id: checkBoxBackground
        x: actionIndicator.width
        y: 0
        z: 5
        implicitWidth: StudioTheme.Values.height
        implicitHeight: StudioTheme.Values.height
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border

        T.Label {
            id: checkedIcon
            x: (parent.width - checkedIcon.width) / 2
            y: (parent.height - checkedIcon.height) / 2
            text: StudioTheme.Constants.tickIcon
            visible: myCheckBox.checkState === Qt.Checked
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.sliderControlSizeMulti
            font.family: StudioTheme.Constants.iconFont.family
        }

        T.Label {
            id: partiallyCheckedIcon
            x: (parent.width - checkedIcon.width) / 2
            y: (parent.height - checkedIcon.height) / 2
            text: StudioTheme.Constants.triState
            visible: myCheckBox.checkState === Qt.PartiallyChecked
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.sliderControlSizeMulti
            font.family: StudioTheme.Constants.iconFont.family
        }
    }

    contentItem: T.Label {
        id: checkBoxLabel
        leftPadding: checkBoxBackground.x + checkBoxBackground.width + myCheckBox.spacing
        rightPadding: 0
        verticalAlignment: Text.AlignVCenter
        text: myCheckBox.text
        font: myCheckBox.font
        color: StudioTheme.Values.themeTextColor
        visible: checkBoxLabel.text !== ""
    }

    states: [
        State {
            name: "default"
            when: myCheckBox.enabled && !myCheckBox.hover
                  && !myCheckBox.pressed && !actionIndicator.hover
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeIconColor
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeIconColor
            }
        },
        State {
            name: "globalHover"
            when: actionIndicator.hover && !myCheckBox.pressed && myCheckBox.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackgroundGlobalHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeIconColor
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeIconColor
            }
        },
        State {
            name: "hover"
            when: myCheckBox.hover && !actionIndicator.hover && !myCheckBox.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackgroundHover
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeIconColor // TODO naming
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeIconColor
            }
        },
        State {
            name: "press"
            when: myCheckBox.hover && myCheckBox.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackgroundInteraction
                border.color: StudioTheme.Values.themeControlOutlineInteraction
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeIconColorInteraction
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeIconColorInteraction
            }
        },
        State {
            name: "disable"
            when: !myCheckBox.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeIconColorDisabled
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeIconColorDisabled
            }
            PropertyChanges {
                target: checkBoxLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
