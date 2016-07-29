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

import QtQuick 2.1
import HelperWidgets 2.0

ButtonRow {
    enabled: anchorBackend.hasParent
    opacity: enabled ? 1 : 0.5

    id: buttonRow

    ButtonRowButton {
        iconSource: "image://icons/anchor-top"
        tooltip: qsTr("Anchor item to the top.")

        property bool topAnchored: anchorBackend.topAnchored
        onTopAnchoredChanged: {
            checked = topAnchored
        }

        onClicked:  {
            if (checked) {
                if (anchorBackend.bottomAnchored)
                    anchorBackend.verticalCentered = false;
                anchorBackend.topAnchored = true;
            } else {
                anchorBackend.topAnchored = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "image://icons/anchor-bottom"
        tooltip: qsTr("Anchor item to the bottom.")

        property bool bottomAnchored: anchorBackend.bottomAnchored
        onBottomAnchoredChanged: {
            checked = bottomAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored)
                    anchorBackend.verticalCentered = false;
                anchorBackend.bottomAnchored = true;
            } else {
                anchorBackend.bottomAnchored = false;
            }
        }

    }

    ButtonRowButton {
        iconSource: "image://icons/anchor-left"
        tooltip: qsTr("Anchor item to the left.")

        property bool leftAnchored: anchorBackend.leftAnchored
        onLeftAnchoredChanged: {
            checked = leftAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.rightAnchored)
                    anchorBackend.horizontalCentered = false;
                anchorBackend.leftAnchored = true;
            } else {
                anchorBackend.leftAnchored = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "image://icons/anchor-right"
        tooltip: qsTr("Anchor item to the right.")

        property bool rightAnchored: anchorBackend.rightAnchored
        onRightAnchoredChanged: {
            checked = rightAnchored
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored)
                    anchorBackend.horizontalCentered = false;
                anchorBackend.rightAnchored = true;
            } else {
                anchorBackend.rightAnchored = false;
            }
        }
    }

    ButtonRowButton {
        enabled: false
    }


    ButtonRowButton {
        iconSource: "image://icons/anchor-fill"
        tooltip: qsTr("Fill parent item.")

        property bool isFilled: anchorBackend.isFilled
        onIsFilledChanged: {
            checked = isFilled
        }

        onClicked: {
            if (checked) {
                anchorBackend.fill();
            } else {
                anchorBackend.resetLayout();
            }
        }
    }

    ButtonRowButton {
        enabled: false
    }

    ButtonRowButton {
        iconSource: "image://icons/anchor-vertical"
        tooltip: qsTr("Anchor item vertically.")

        property bool verticalCentered: anchorBackend.verticalCentered;
        onVerticalCenteredChanged: {
            checked = verticalCentered
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.topAnchored && anchorBackend.bottomAnchored) {
                    anchorBackend.topAnchored = false;
                    anchorBackend.bottomAnchored = false;
                }
                anchorBackend.verticalCentered = true;
            } else {
                anchorBackend.verticalCentered = false;
            }
        }
    }

    ButtonRowButton {
        iconSource: "image://icons/anchor-horizontal"
        tooltip: qsTr("Anchor item horizontally.")

        property bool horizontalCentered: anchorBackend.horizontalCentered;
        onHorizontalCenteredChanged: {
            checked = horizontalCentered
        }

        onClicked: {
            if (checked) {
                if (anchorBackend.leftAnchored && anchorBackend.rightAnchored) {
                    anchorBackend.leftAnchored = false;
                    anchorBackend.rightAnchored = false;
                }
                anchorBackend.horizontalCentered = true;
            } else {
                anchorBackend.horizontalCentered = false;
            }
        }
    }
}
