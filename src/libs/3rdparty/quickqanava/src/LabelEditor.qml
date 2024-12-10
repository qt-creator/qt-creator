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
// \file	LabelEditor.qml
// \author	benoit@destrat.io
// \date	2015 11 30
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Controls

/*! \brief Reusable component for visual edition of node and groups labels with on-demand loading.
 *
 * Initialize 'target' property with the node or group that should be edited, editor is invisible by default,
 * complex label edition component is loaded only when editor \c visible is set to true.
 *
 * \note LabelEditor is basically a loader with an internal edition component using
 * a Quick Controls 2 TextField that is loaded only when visible property is sets to true. Internal
 * editor is not unloaded when editor visibility is set to false.
 *
 * Exemple use on an user defined custom node:
 * \code
 * Qan.Node {
 *   id: rectNode
 *   width: 110; height: 50
 *   Connections {
 *    target: node
 *     onNodeDoubleClicked: labelEditor.visible = true
 *   }
 *   LabelEditor {
 *     id: nodeLabelEditor
 *     anchors.fill: parent
 *     anchors.margins: background.radius / 2
 *     target: parent.node
 *   }
 * }
 * \endcode
 *
 * More consistent sample is available in Qan.RectNodeTemplate component.
 */
Loader {
    id: labelEditorLoader
    visible: false

    //! Editor target node or group (or any Qan primitive with a \c label property).
    property var    target: undefined

    property real   fontPixelSize: 11

    onVisibleChanged: {
        if (visible && !item)
            sourceComponent = labelEditorComponent
        if (item &&
            visible)
            showLabelEditor()
    }
    onItemChanged: {
        if (item)             // If item is non null, it's creation has been request,
            showLabelEditor()   // force show label edition
    }
    function showLabelEditor() {
        if (item) {
            if (!visible)
                visible = true
            item.forceActiveFocus()
            item.selectAll()
        }
    }
    Component {
        id: labelEditorComponent
        TextField {
            id: labelTextField
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            text: target ? target.label : ""
            font.bold: false
            font.pixelSize: 22
            onAccepted: {
                if (target &&
                    text.length !== 0)
                    target.label = text   // Do not allow empty labels
                focus = false;          // Release focus once the label has been edited
            }
            onEditingFinished: labelEditorLoader.visible = false
            onActiveFocusChanged: {
                if (target &&
                    text !== target.label)  // Ensure that last edition text is removed
                    text = target.label       // for exemple if edition has been interrupted in a focus change
            }
            background: Rectangle {
                color: Qt.rgba(0., 0., 0., 0.)  // Disable editor background
            }
            Keys.onEscapePressed: {
                // Cancel edition on escape
                labelEditorLoader.visible = false;
            }
        }
    }
} // Loader
