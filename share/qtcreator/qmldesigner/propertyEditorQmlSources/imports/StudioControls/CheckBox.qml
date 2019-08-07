/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

T.CheckBox {
    id: myCheckBox

    property alias actionIndicator: actionIndicator

    property bool hover: myCheckBox.hovered
    property bool edit: false

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.squareComponentWidth
    property real __actionIndicatorHeight: StudioTheme.Values.height

    property alias labelVisible: checkBoxLabel.visible
    property alias labelColor: checkBoxLabel.color

    font.pixelSize: StudioTheme.Values.myFontSize

    implicitWidth: Math.max(
                       implicitBackgroundWidth + leftInset + rightInset,
                       implicitContentWidth + leftPadding + rightPadding
                       + implicitIndicatorWidth + spacing + actionIndicator.width)
    implicitHeight: Math.max(
                        implicitBackgroundHeight + topInset + bottomInset,
                        implicitContentHeight + topPadding + bottomPadding,
                        implicitIndicatorHeight + topPadding + bottomPadding)

    spacing: StudioTheme.Values.checkBoxSpacing
    hoverEnabled: true
    activeFocusOnTab: false

    ActionIndicator {
        id: actionIndicator
        myControl: myCheckBox // TODO global hover issue. Can be solved with extra property in ActionIndicator
        width: actionIndicator.visible ? __actionIndicatorWidth : 0
        height: actionIndicator.visible ? __actionIndicatorHeight : 0
    }

    indicator: Rectangle {
        id: checkBoxBackground
        x: actionIndicator.x + actionIndicator.width
           - (actionIndicator.visible ? StudioTheme.Values.border : 0)
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
        visible: text !== ""
    }

    states: [
        State {
            name: "default"
            when: myCheckBox.enabled && !myCheckBox.hovered
                  && !myCheckBox.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackground
            }
        },
        State {
            name: "hovered"
            when: myCheckBox.hovered && !myCheckBox.pressed
                  && !actionIndicator.hover
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeHoverHighlight
            }
        },
        State {
            name: "pressed"
            when: myCheckBox.hovered && myCheckBox.pressed
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeFocusEdit
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
            when: !myCheckBox.enabled
            PropertyChanges {
                target: checkBoxBackground
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: checkedIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: partiallyCheckedIcon
                color: StudioTheme.Values.themeTextColorDisabled
            }
            PropertyChanges {
                target: checkBoxLabel
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
