/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.0
import HelperWidgets 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as Controls

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Layout")

    LayoutPoperties {
        id: layoutPoperties
        visible: anchorBackend.isInLayout
    }

    ColumnLayout {
        visible: !anchorBackend.isInLayout
        width: parent.width
        Label {
            text: qsTr("Anchors")
        }

        AnchorButtons {
        }

        AnchorRow {
            enabled: anchorBackend.topAnchored;
            iconSource: "../HelperWidgets/images/anchor-top.png"
            anchorMargin: backendValues.anchors_topMargin
            targetName: anchorBackend.topTarget
            onTargetChanged: {
                anchorBackend.topTarget = currentText
            }
            relativeTarget: anchorBackend.relativeAnchorTargetTop
            verticalAnchor: true

            onSameEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetTop = AnchorBindingProxy.SameEdge
            }
            onOppositeEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetTop = AnchorBindingProxy.OppositeEdge
            }

            onCenterButtonClicked: {
                anchorBackend.relativeAnchorTargetTop = AnchorBindingProxy.Center
            }
        }

        AnchorRow {
            enabled: anchorBackend.bottomAnchored;
            iconSource: "../HelperWidgets/images/anchor-bottom.png"
            anchorMargin: backendValues.anchors_bottomMargin
            targetName: anchorBackend.bottomTarget
            onTargetChanged: {
                anchorBackend.bottomTarget = currentText
            }
            relativeTarget: anchorBackend.relativeAnchorTargetBottom
            verticalAnchor: true
            invertRelativeTargets: true

            onSameEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetBottom = AnchorBindingProxy.SameEdge
            }
            onOppositeEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetBottom = AnchorBindingProxy.OppositeEdge
            }

            onCenterButtonClicked: {
                anchorBackend.relativeAnchorTargetBottom = AnchorBindingProxy.Center
            }
        }

        AnchorRow {
            enabled: anchorBackend.leftAnchored;
            iconSource: "../HelperWidgets/images/anchor-left.png"
            anchorMargin: backendValues.anchors_leftMargin
            targetName: anchorBackend.leftTarget
            onTargetChanged: {
                anchorBackend.leftTarget = currentText
            }
            relativeTarget: anchorBackend.relativeAnchorTargetLeft
            verticalAnchor: false

            onSameEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetLeft = AnchorBindingProxy.SameEdge
            }
            onOppositeEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetLeft = AnchorBindingProxy.OppositeEdge
            }

            onCenterButtonClicked: {
                anchorBackend.relativeAnchorTargetLeft = AnchorBindingProxy.Center
            }
        }

        AnchorRow {
            enabled: anchorBackend.rightAnchored;
            iconSource: "../HelperWidgets/images/anchor-right.png"
            anchorMargin: backendValues.anchors_rightMargin
            targetName: anchorBackend.rightTarget
            onTargetChanged: {
                anchorBackend.rightTarget = currentText
            }
            relativeTarget: anchorBackend.relativeAnchorTargetRight
            verticalAnchor: false
            invertRelativeTargets: true

            onSameEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetRight = AnchorBindingProxy.SameEdge
            }
            onOppositeEdgeButtonClicked: {
                anchorBackend.relativeAnchorTargetRight = AnchorBindingProxy.OppositeEdge
            }

            onCenterButtonClicked: {
                anchorBackend.relativeAnchorTargetRight = AnchorBindingProxy.Center
            }
        }

        AnchorRow {
            showAlternativeTargets: false
            enabled: anchorBackend.horizontalCentered;
            iconSource: "../HelperWidgets/images/anchor-horizontal.png"
            anchorMargin: backendValues.anchors_horizontalCenterOffset
            targetName: anchorBackend.horizontalTarget
            onTargetChanged: {
                anchorBackend.horizontalTarget = currentText
            }
            verticalAnchor: true
        }

        AnchorRow {
            showAlternativeTargets: false
            enabled: anchorBackend.verticalCentered;
            iconSource: "../HelperWidgets/images/anchor-vertical.png"
            anchorMargin: backendValues.anchors_verticalCenterOffset
            targetName: anchorBackend.verticalTarget
            onTargetChanged: {
                 anchorBackend.verticalTarget = currentText
            }
            verticalAnchor: false
        }

    }
}
