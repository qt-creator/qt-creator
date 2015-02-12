/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
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
