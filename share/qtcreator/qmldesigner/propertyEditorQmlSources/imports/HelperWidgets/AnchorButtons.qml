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

    StudioControls.ButtonGroup { id: group }

    AbstractButton {
        property bool topAnchored: anchorBackend.topAnchored

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorTop
        tooltip: qsTr("Anchor component to the top.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorBottom
        tooltip: qsTr("Anchor component to the bottom.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorLeft
        tooltip: qsTr("Anchor component to the left.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorRight
        tooltip: qsTr("Anchor component to the right.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.anchorFill
        tooltip: qsTr("Fill parent component.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.centerVertical
        tooltip: qsTr("Anchor component vertically.")

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

        checkable: true
        buttonIcon: StudioTheme.Constants.centerHorizontal
        tooltip: qsTr("Anchor component horizontally.")

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
