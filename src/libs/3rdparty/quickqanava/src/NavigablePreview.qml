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

import QtQuick

import QuickQanava as Qan

/*! \brief Concrete component for qan::NavigablePreview interface.
 *
 */
Qan.AbstractNavigablePreview {
    id: preview
    clip: true

    // PUBLIC /////////////////////////////////////////////////////////////////

    //! Overlay item could be used to display a user defined item (for example an heat map image) between the background and the current visible window rectangle.
    property var    overlay: overlayItem

    //! Color for the visible window rect border (default to red).
    property color  viewWindowColor: Qt.rgba(1, 0, 0, 1)

    //! Show or hide the target navigable content as a background image (default to true).
    property alias  backgroundPreviewVisible: sourcePreview.visible

    //! Initial (and minimum) scene rect (should usually fit your initial screen size).
    property rect   initialRect: Qt.rect(-1280 / 2., -720 / 2.,
                                         1280, 720)

    // PRIVATE ////////////////////////////////////////////////////////////////
    onSourceChanged: {
        if (source &&
            source.containerItem) {
            resetVisibleWindow()
            source.containerItemModified.connect(updatePreview)
            source.onWidthChanged.connect(updatePreview)
            source.onHeightChanged.connect(updatePreview)
            // Emitted only on user initiated navigable view changes.
            source.onNavigated.connect(updatePreview)
            sourcePreview.sourceItem = source.containerItem
        } else
            sourcePreview.sourceItem = undefined
        updatePreview()
        updateVisibleWindow()
    }

    ShaderEffectSource {
        id: sourcePreview
        anchors.fill: parent
        anchors.margins: 0
        live: true
        recursive: false
        textureSize: Qt.size(width, height)
    }

    function updatePreview() {
        if (!source)
            return
        // Note 20240823: Prevent updating the view while it is dragged
        // to avoid "Maximum call stack size exceeded"
        if (viewWindowController.pressedButtons & Qt.LeftButton ||
            viewWindowController.active)
            return;
        const r = computeSourceRect()
        if (r &&
            r.width > 0. &&
            r.height > 0) {
            sourcePreview.sourceRect = r
            viewWindow.visible = true
            updateVisibleWindow(r)
        } else
            viewWindow.visible = false
    }

    function computeSourceRect(rect) {
        if (!source)
            return undefined
        if (!preview.source ||
            !preview.source.containerItem ||
            sourcePreview.sourceItem !== preview.source.containerItem)
            return undefined

        // Scene rect is union of initial rect and children rect.
        let cr = preview.source.containerItem.childrenRect
        let r = preview.rectUnion(cr, preview.initialRect)
        return r
    }

    // Reset viewWindow rect to preview dimension (taking rectangle border into account)
    function    resetVisibleWindow() {
        const border = viewWindow.border.width
        const border2 = border * 2
        viewWindow.x = border
        viewWindow.y = border
        viewWindow.width = preview.width - border2
        viewWindow.height = preview.height - border2
    }

    function    updateVisibleWindow(r) {
        // r is previewed rect in source.containerItem Cs
        if (!preview)
            return
        if (!source) {  // Reset the window when source is invalid
            preview.resetVisibleWindow()
            return
        }
        var containerItem = source.containerItem
        if (!containerItem) {
            preview.resetVisibleWindow()
            return
        }
        if (!r)
            return

        // r is content rect in scene Cs
        // viewR is window rect in content rect Cs
        // Window is viewR in preview Cs

        const border = viewWindow.border.width
        const border2 = border * 2.

        // map r to preview
        // map viewR to preview
        // Apply scaling from r to preview
        const viewR = preview.source.mapToItem(preview.source.containerItem,
                                               Qt.rect(0, 0,
                                                       preview.source.width,
                                                       preview.source.height))
        var previewXRatio = preview.width / r.width
        var previewYRatio = preview.height / r.height

        viewWindow.visible = true
        viewWindow.x = (previewXRatio * (viewR.x - r.x)) + border
        viewWindow.y = (previewYRatio * (viewR.y - r.y)) + border
        // Set a minimum view window size to avoid users beeing lost in large graphs
        viewWindow.width = Math.max(12, (previewXRatio * viewR.width) - border2)
        viewWindow.height = Math.max(9, (previewYRatio * viewR.height) - border2)

        visibleWindowChanged(Qt.rect(viewWindow.x / preview.width,
                                  viewWindow.y / preview.height,
                                  viewWindow.width / preview.width,
                                  viewWindow.height / preview.height),
                             source.zoom);
    }

    // Map from preview Cs to graph Cs
    function    mapFromPreview(p) {
        if (!p)
            return;
        // preview window origin is (r.x, r.y)
        let r = sourcePreview.sourceRect
        var previewXRatio = r.width / preview.width
        var previewYRatio = r.height / preview.height
        let sourceP = Qt.point((p.x * previewXRatio) + r.x,
                               (p.y * previewYRatio) + r.y)
        return sourceP
    }

    Item {
        id: overlayItem
        anchors.fill: parent; anchors.margins: 0
    }
    Rectangle {
        id: viewWindow
        z: 1
        color: Qt.rgba(0, 0, 0, 0)
        border.color: viewWindowColor
        border.width: 2
        onXChanged: viewWindowDragged()
        onYChanged: viewWindowDragged()
        function viewWindowDragged() {
            if (source &&
                (viewWindowController.pressedButtons & Qt.LeftButton ||
                 viewWindowController.active)) {
                // Convert viewWindow coordinate to source graph view CCS
                let sceneP = mapFromPreview(Qt.point(viewWindow.x, viewWindow.y))
                source.moveTo(sceneP)
            }
        }
        MouseArea { // Manage dragging of "view window"
            id: viewWindowController
            anchors.fill: parent
            drag.target: viewWindow
            drag.threshold: 1.
            drag.minimumX: 0    // Do not allow dragging outside preview area
            drag.minimumY: 0
            drag.maximumX: Math.max(0, preview.width - viewWindow.width)
            drag.maximumY: Math.max(0, preview.height - viewWindow.height)
            acceptedButtons: Qt.LeftButton
            enabled: true
            cursorShape: Qt.SizeAllCursor
            // Note 20221221: Surprisingly, onClicked and onDoubleClicked are activated without interferences
            // while dragging is enabled. Activate zoom on click and double click just as in navigation controller,
            // usefull when scene is fully de-zommed and view window take the full control space.
            Timer {  // See Note 20221204
                id: viewWindowTimer
                interval: 100
                running: false
                property point p: Qt.point(0, 0)
                onTriggered: {
                    let sceneP = mapFromPreview(Qt.point(p.x, p.y))
                    source.centerOnPosition(sceneP)
                    updatePreview()
                }
            }
            onClicked: (mouse) => {
                let p = mapToItem(preview, Qt.point(mouse.x, mouse.y))
                viewWindowTimer.p = p
                viewWindowTimer.start()
                mouse.accepted = true
            }
            onDoubleClicked: (mouse) => {
                viewWindowTimer.stop()
                let p = mapToItem(preview, Qt.point(mouse.x, mouse.y))
                let sceneP = mapFromPreview(p)
                source.centerOnPosition(sceneP)
                source.zoom = 1.0
                mouse.accepted = true
                updatePreview()
            }
        }
    }  // Rectangle: viewWindow
    MouseArea {  // Manage move on click and zoom on double click
        id: navigationController
        anchors.fill: parent
        z: 0
        acceptedButtons: Qt.LeftButton
        enabled: true
        cursorShape: Qt.CrossCursor
        // Note 20221204: Hack for onClicked/onDoubleClicked(), see:
        // https://forum.qt.io/topic/103829/providing-precedence-for-ondoubleclicked-over-onclicked-in-qml/2
        // onClicked trigger a centerOnPosition(), hidding this mouse area
        // with viewWindow, thus not triggering any double click
        Timer {
            id: timer
            interval: 100
            running: false
            property point p: Qt.point(0, 0)
            onTriggered: {
                let sceneP = mapFromPreview(Qt.point(p.x, p.y))
                source.centerOnPosition(sceneP)
                updatePreview()
            }
        }
        onClicked: (mouse) => {
            timer.p = Qt.point(mouse.x, mouse.y)
            timer.start()
            mouse.accepted = true
        }
        onDoubleClicked: (mouse) =>{
            timer.stop()
            let sceneP = mapFromPreview(Qt.point(mouse.x, mouse.y))
            source.centerOnPosition(sceneP)
            source.zoom = 1.0
            mouse.accepted = true
            updatePreview()
        }
    }
}  // Qan.AbstractNavigablePreview: preview
