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
import QtQuick.Layouts 1.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Layout")
    spacing: StudioTheme.Values.sectionRowSpacing + 6

    LayoutProperties {
        id: layoutPoperties
        visible: anchorBackend.isInLayout
    }

    SectionLayout {
        visible: !anchorBackend.isInLayout

        PropertyLabel { text: qsTr("Anchors") }

        SecondColumnLayout {
            Spacer {
                implicitWidth: StudioTheme.Values.actionIndicatorWidth
            }

            AnchorButtons {}

            ExpandingSpacer {}
        }
    }

    AnchorRow {
        visible: anchorBackend.topAnchored
        iconSource: StudioTheme.Constants.anchorTop
        anchorMargin: backendValues.anchors_topMargin
        targetName: anchorBackend.topTarget
        relativeTarget: anchorBackend.relativeAnchorTargetTop
        verticalAnchor: true

        onTargetChanged: {
            anchorBackend.topTarget = currentText
        }
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
        visible: anchorBackend.bottomAnchored
        iconSource: StudioTheme.Constants.anchorBottom
        anchorMargin: backendValues.anchors_bottomMargin
        targetName: anchorBackend.bottomTarget
        relativeTarget: anchorBackend.relativeAnchorTargetBottom
        verticalAnchor: true
        invertRelativeTargets: true

        onTargetChanged: {
            anchorBackend.bottomTarget = currentText
        }
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
        visible: anchorBackend.leftAnchored
        iconSource: StudioTheme.Constants.anchorLeft
        anchorMargin: backendValues.anchors_leftMargin
        targetName: anchorBackend.leftTarget
        relativeTarget: anchorBackend.relativeAnchorTargetLeft
        verticalAnchor: false

        onTargetChanged: {
            anchorBackend.leftTarget = currentText
        }
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
        visible: anchorBackend.rightAnchored
        iconSource: StudioTheme.Constants.anchorRight
        anchorMargin: backendValues.anchors_rightMargin
        targetName: anchorBackend.rightTarget
        relativeTarget: anchorBackend.relativeAnchorTargetRight
        verticalAnchor: false
        invertRelativeTargets: true

        onTargetChanged: {
            anchorBackend.rightTarget = currentText
        }
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
        visible: anchorBackend.horizontalCentered
        iconSource: StudioTheme.Constants.centerHorizontal
        anchorMargin: backendValues.anchors_horizontalCenterOffset
        targetName: anchorBackend.horizontalTarget
        verticalAnchor: false
        buttonRow.visible: false

        onTargetChanged: {
            anchorBackend.horizontalTarget = currentText
        }
    }

    AnchorRow {
        showAlternativeTargets: false
        visible: anchorBackend.verticalCentered
        iconSource: StudioTheme.Constants.centerVertical
        anchorMargin: backendValues.anchors_verticalCenterOffset
        targetName: anchorBackend.verticalTarget
        verticalAnchor: true
        buttonRow.visible: false

        onTargetChanged: {
             anchorBackend.verticalTarget = currentText
        }
    }
}
