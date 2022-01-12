/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.ButtonRow {
    id: buttonRow

    enabled: anchorBackend.hasParent && isBaseState
    actionIndicatorVisible: false

    function pickToolTip(defaultText) {
        if (!anchorBackend.hasParent)
            return qsTr("Anchors can only be applied to child items.")
        else if (!isBaseState)
            return qsTr("Anchors can only be applied to the base state.")

        return defaultText
    }

    StudioControls.ButtonGroup { id: group }

    AbstractButton {
        property bool topAnchored: anchorBackend.topAnchored
        property string toolTipText: qsTr("Anchor component to the top.")

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorTop
        tooltip: buttonRow.pickToolTip(toolTipText)

        onTopAnchoredChanged: checked = topAnchored

        onClicked: {
            if (checked) {
                if (anchorBackend.bottomAnchored)
                    anchorBackend.verticalCentered = false

                anchorBackend.topAnchored = true
            } else {
                anchorBackend.topAnchored = false
            }
        }
    }

    AbstractButton {
        property bool bottomAnchored: anchorBackend.bottomAnchored
        property string toolTipText: qsTr("Anchor component to the bottom.")

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorBottom
        tooltip: buttonRow.pickToolTip(toolTipText)

        onBottomAnchoredChanged: checked = bottomAnchored

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored)
                    anchorBackend.verticalCentered = false

                anchorBackend.bottomAnchored = true
            } else {
                anchorBackend.bottomAnchored = false
            }
        }
    }

    AbstractButton {
        property bool leftAnchored: anchorBackend.leftAnchored
        property string toolTipText: qsTr("Anchor component to the left.")

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorLeft
        tooltip: buttonRow.pickToolTip(toolTipText)

        onLeftAnchoredChanged: checked = leftAnchored

        onClicked: {
            if (checked) {
                if (anchorBackend.rightAnchored)
                    anchorBackend.horizontalCentered = false

                anchorBackend.leftAnchored = true
            } else {
                anchorBackend.leftAnchored = false
            }
        }
    }

    AbstractButton {
        property bool rightAnchored: anchorBackend.rightAnchored
        property string toolTipText: qsTr("Anchor component to the right.")

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorRight
        tooltip: buttonRow.pickToolTip(toolTipText)

        onRightAnchoredChanged: checked = rightAnchored

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored)
                    anchorBackend.horizontalCentered = false

                anchorBackend.rightAnchored = true
            } else {
                anchorBackend.rightAnchored = false
            }
        }
    }

    Spacer {
        implicitWidth: 16 + 2 * StudioTheme.Values.border
    }

    AbstractButton {
        property bool isFilled: anchorBackend.isFilled
        property string toolTipText: qsTr("Fill parent component.")

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorFill
        tooltip: buttonRow.pickToolTip(toolTipText)

        onIsFilledChanged: checked = isFilled

        onClicked: {
            if (checked)
                anchorBackend.fill()
            else
                anchorBackend.resetLayout()
        }
    }

    Spacer {
        implicitWidth: 16 + 2 * StudioTheme.Values.border
    }

    AbstractButton {
        property bool verticalCentered: anchorBackend.verticalCentered
        property string toolTipText: qsTr("Anchor component vertically.")

        checkable: true
        buttonIcon: StudioTheme.Constants.centerVertical
        tooltip: buttonRow.pickToolTip(toolTipText)

        onVerticalCenteredChanged: checked = verticalCentered

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored && anchorBackend.bottomAnchored) {
                    anchorBackend.topAnchored = false
                    anchorBackend.bottomAnchored = false
                }
                anchorBackend.verticalCentered = true
            } else {
                anchorBackend.verticalCentered = false
            }
        }
    }

    AbstractButton {
        property bool horizontalCentered: anchorBackend.horizontalCentered
        property string toolTipText: qsTr("Anchor component horizontally.")

        checkable: true
        buttonIcon: StudioTheme.Constants.centerHorizontal
        tooltip: buttonRow.pickToolTip(toolTipText)

        onHorizontalCenteredChanged: checked = horizontalCentered

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored && anchorBackend.rightAnchored) {
                    anchorBackend.leftAnchored = false
                    anchorBackend.rightAnchored = false
                }
                anchorBackend.horizontalCentered = true
            } else {
                anchorBackend.horizontalCentered = false
            }
        }
    }
}
