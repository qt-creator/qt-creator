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
// \file	VerticalDock.qml
// \author	benoit@destrat.io
// \date	2017 08 28
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Layouts

import QuickQanava as Qan

ColumnLayout {
    id: verticalDock
    spacing: 20
    z: 1.5   // Selection item z=1.0, dock must be on top of selection

    property var hostNodeItem: undefined
    property int dockType: -1
    property int leftMargin: 7
    property int rightMargin: 7

    // Note 20220426: Changing dock position actually do not modify
    // docked port position, so no edge update is triggered, force update
    // manually (fix #145)
    onXChanged: hostNodeItem.updatePortsEdges()
    onYChanged: hostNodeItem.updatePortsEdges()

    states: [
        State {
            name: "left"
            when: hostNodeItem !== null && hostNodeItem !== undefined &&
                  dockType === Qan.NodeItem.Left

            AnchorChanges {
                target: verticalDock
                anchors {
                    right: hostNodeItem.left
                    verticalCenter: hostNodeItem.verticalCenter
                }
            }

            PropertyChanges {
                target: verticalDock
                rightMargin: verticalDock.rightMargin
            }
        },
        State {
            name: "right"
            when: verticalDock.hostNodeItem !== undefined &&
                  dockType === Qan.NodeItem.Right

            AnchorChanges {
                target: verticalDock
                anchors {
                    left: hostNodeItem.right
                    verticalCenter: hostNodeItem.verticalCenter
                }
            }

            PropertyChanges {
                target: verticalDock
                leftMargin: verticalDock.leftMargin
            }
        }
    ]
}
