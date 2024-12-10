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
// \file	Port.qml
// \author	benoit@destrat.io
// \date	2017 08 12
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Controls

import QuickQanava as Qan

Qan.PortItem {
    id: portItem
    width: 16
    height: 16

    states: [
        State {
            name: "left"
            when: dockType === Qan.NodeItem.Left

            AnchorChanges {
                target: labelPane
                anchors {
                    left: undefined
                    top: undefined
                    right: contentItem.right
                    bottom: contentItem.top
                    horizontalCenter: undefined
                }
            }
        },
        State {
            name: "top"
            when: dockType === Qan.NodeItem.Top

            AnchorChanges {
                target: labelPane
                anchors {
                    left: undefined
                    top: undefined
                    right: undefined
                    bottom: contentItem.top
                    horizontalCenter: contentItem.horizontalCenter
                }
            }
        },
        State {
            name: "right"
            when: dockType === Qan.NodeItem.Right

            AnchorChanges {
                target: labelPane
                anchors {
                    left: contentItem.left
                    top: undefined
                    right: undefined
                    bottom: contentItem.top
                    horizontalCenter: undefined
                }
            }
        },
        State {
            name: "bottom"
            when: dockType === Qan.NodeItem.Bottom

            AnchorChanges {
                target: labelPane
                anchors {
                    left: undefined
                    top: portItem.bottom
                    right: undefined
                    bottom: undefined
                    horizontalCenter: portItem.horizontalCenter
                }
            }

            PropertyChanges {
                target: labelPane
                width: label.implicitWidth
                height: label.implicitHeight
            }
        }
    ]

    Rectangle {
        id: contentItem
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border {
            color: "lightblue"
            width: 3
        }
    }

    Pane {
        id: labelPane
        opacity: 0.80
        padding: 0
        z: 2
        width: label.implicitWidth
        height: label.implicitHeight

        Label {
            id: label
            z: 3
            text: portItem.label
            visible: true
        }
    }
}

