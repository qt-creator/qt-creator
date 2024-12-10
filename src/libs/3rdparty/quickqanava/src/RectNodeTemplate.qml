/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	RectNodeTemplate.qml
// \author	benoit@destrat.io
// \date	2015 11 30
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QuickQanava as Qan

/*! \brief Default template component for building a custom rectangular qan::Node item.
 *
 * Node with custom content definition using "templates" is described in \ref qanavacustom
 */
Item {
    id: template

    // PUBLIC /////////////////////////////////////////////////////////////////
    property var            nodeItem : undefined
    default property alias  children : contentLayout.children

    // PRIVATE ////////////////////////////////////////////////////////////////
    onNodeItemChanged: {
        if (delegateLoader?.item?.nodeItem)
            delegateLoader.item.nodeItem = nodeItem
    }
    onWidthChanged: updateDefaultBoundingShape()
    onHeightChanged: updateDefaultBoundingShape()
    function updateDefaultBoundingShape() {
        if (nodeItem)
            nodeItem.setDefaultBoundingShape()
    }

    readonly property real   backRadius: nodeItem?.style?.backRadius ?? 4.
    Loader {
        id: delegateLoader
        anchors.fill: parent
        source: {
            if (!nodeItem?.style)     // Defaul to solid no effect with unconfigured nodes
                return "qrc:/QuickQanava/RectSolidBackground.qml";
            switch (nodeItem.style.fillType) {  // Otherwise, select the delegate according to current style configuration
            case Qan.NodeStyle.FillSolid:
                switch (nodeItem.style.effectType) {
                case Qan.NodeStyle.EffectNone:   return "qrc:/QuickQanava/RectSolidBackground.qml";
                case Qan.NodeStyle.EffectShadow: return "qrc:/QuickQanava/RectSolidShadowBackground.qml";
                case Qan.NodeStyle.EffectGlow:   return "qrc:/QuickQanava/RectSolidGlowBackground.qml";
                }
                break;
            case Qan.NodeStyle.FillGradient:
                switch (nodeItem.style.effectType) {
                case Qan.NodeStyle.EffectNone:   return "qrc:/QuickQanava/RectGradientBackground.qml";
                case Qan.NodeStyle.EffectShadow: return "qrc:/QuickQanava/RectGradientShadowBackground.qml";
                case Qan.NodeStyle.EffectGlow:   return "qrc:/QuickQanava/RectGradientGlowBackground.qml";
                }
                break;
            } // case fillType
        }
        onItemChanged: {
            if (item)
                item.style = template.nodeItem?.style || undefined
        }
    }
    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: backRadius / 2.
        spacing: 0
        visible: !labelEditor.visible
        Label {
            id: nodeLabel
            Layout.fillWidth: true
            Layout.fillHeight: contentLayout.children.length === 0
            Layout.preferredHeight: contentHeight
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            textFormat: Text.PlainText
            text: nodeItem && nodeItem.node ? nodeItem.node.label : ''
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            maximumLineCount: 3 // Must be set, otherwise elide don't work and we end up with single line text
            elide: Text.ElideRight
            wrapMode: Text.Wrap
        }
        Item {
            id: contentLayout
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true;
            Layout.fillHeight: true
            visible: contentLayout.children.length > 0  // Hide if the user has not added any content
        }
    }
    Connections {
        target: nodeItem
        function onNodeDoubleClicked() { labelEditor.visible = true }
    }
    LabelEditor {
        id: labelEditor
        anchors.fill: parent
        anchors.margins: backRadius / 2.
        target: template.nodeItem.node
        visible: false
    }
} // Item: template
