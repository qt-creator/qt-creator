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
import QtQuick.Controls
import QtQuick.Effects

/*! \brief Visual graph preview.
 *
 */
Control {
    id: graphPreview

    // PUBLIC /////////////////////////////////////////////////////////////////
    width: 200
    height: 135

    //! Source Qan.GraphView that should be previewed.
    property var    source: undefined

    property alias  viewWindowColor: navigablePreview.viewWindowColor

    // Preview background panel opacity (default to 0.9).
    property alias  previewOpactity: previewBackground.opacity

    //! Initial (and minimum) scene rect (should usually fit your initial screen size).
    property alias  initialRect: navigablePreview.initialRect

    // PRIVATE ////////////////////////////////////////////////////////////////
    padding: 0

    property real   graphRatio: source ? (source.containerItem.childrenRect.width /
                                         source.containerItem.childrenRect.height) :
                                         1.
    property real   previewRatio: source ? (source.width / source.height) : 1.0
    onGraphRatioChanged: updateNavigablePreviewSize()
    onPreviewRatioChanged: updateNavigablePreviewSize()
    onSourceChanged: updateNavigablePreviewSize()
    onWidthChanged: updateNavigablePreviewSize()
    onHeightChanged: updateNavigablePreviewSize()

    function updateNavigablePreviewSize() {
        // Update the navigable preview width/height such that it's aspect ratio
        // is correct but fit in this graph preview size.
        // Algorithm:
        // 1. Compute navigable preview height (nph) given graph width (gw) and graphRatio.
        // 2. If navigable preview height (nph) < preview height (ph), then use graphRatio to
        //    generate nph.
        // 3. Else compute navigable preview width using previewRatio and fix nph to ph.
        if (!source || !source.containerItem)
            return
        const pw = graphPreview.width
        const ph = graphPreview.height

        const gw = source.containerItem.childrenRect.width
        const gh = source.containerItem.childrenRect.height

        // 1.
        let sw = pw / gw
        let sh = ph / gh
        let npw = 0.
        let nph = pw * graphRatio
        if (nph < ph) {
            // 2.
            npw = pw
        } else {
            // 3.
            nph = ph
            npw = ph * graphRatio
        }
        // If npw is larger than actual preview width (pw), scale nph
        if (npw > pw)
            nph = nph * (pw / npw)

        // Secure with boundary Check
        navigablePreview.width = Math.min(npw, pw)
        navigablePreview.height = Math.min(nph, ph)
    }

    opacity: 0.8
    hoverEnabled: true

    z: 3    // Avoid tooltips beeing generated on top of preview
    MultiEffect {
        source: previewBackground
        anchors.centerIn: previewBackground
        visible: true
        width: previewBackground.width + 5
        height: previewBackground.height + 5
        blurEnabled: true
        blurMax: 16
        blur: 0.6
        blurMultiplier: 0.1
        colorization: 1.0
        colorizationColor: Qt.rgba(0.7, 0.7, 0.7, 0.9)
    }
    Pane {
        id: previewBackground
        anchors.fill: parent
        opacity: 0.9
        padding: 1
        clip: true
        Label {
            x: 4
            y: 2
            text: source ? ((source.zoom * 100).toFixed(1) + "%") :
                           ''
            font.pixelSize: 11
        }
        NavigablePreview {
            id: navigablePreview
            anchors.centerIn: parent
            source: graphPreview.source
        }  // Qan.NavigablePreview
    }
}  // Control graph preview
