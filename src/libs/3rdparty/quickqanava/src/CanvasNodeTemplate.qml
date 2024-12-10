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
// \file	NodeCanvasTemplate.qml
// \author	benoit@destrat.io
// \date	2015 11 30
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Effects

import QuickQanava as Qan

/*! \brief Default component template for building a custom qan::Node Item with an arbitrary geometry drawn into a JS Canvas2D.
 *
 * Node with custom content definition using "templates" is described in \ref qanavacustom
 */
Item {
    id: template
    property         var    nodeItem : undefined
    property         alias  symbol   : nodeSymbol.sourceComponent
    default property alias  children : templateContentLayout.children

    function requestPaint() {
        if (nodeSymbol.item)
            nodeSymbol.item.requestPaint();
    }
    Loader {    // Node symbol is node background
        id: nodeSymbol
        anchors.fill: parent
    }

    MultiEffect {
        id: backgroundShadow
        source: nodeSymbol.item
        anchors.fill: parent
        anchors.margins: 1
        autoPaddingEnabled: false
        readonly property real shadowOffset: nodeItem.style.effectRadius
        paddingRect: Qt.rect(shadowOffset, shadowOffset,
                             parent.width + shadowOffset,
                             parent.height + shadowOffset)
        shadowEnabled: true
        shadowColor: nodeItem.style.effectColor
        blurMax: 8.
        shadowHorizontalOffset: shadowOffset
        shadowVerticalOffset: shadowOffset
    }

    ColumnLayout {
        id: nodeLayout
        anchors.fill: parent
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        visible: !labelEditor.visible
        Label {
            id: nodeLabel
            Layout.fillWidth: true;
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.margins: 5
            width: parent.width
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            text: nodeItem.node.label
            wrapMode: Text.Wrap;    elide: Text.ElideRight; maximumLineCount: 4
        }
        Item {
            id: templateContentLayout
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: templateContentLayout.children.length > 0  // Hide if the user has not added any content
        }
    }
    Connections {
        target: nodeItem
        function onNodeDoubleClicked() { labelEditor.visible = true }
    }
    LabelEditor {
        id: labelEditor
        anchors.fill: parent
        anchors.margins: 4
        target: parent.nodeItem
    }
}
