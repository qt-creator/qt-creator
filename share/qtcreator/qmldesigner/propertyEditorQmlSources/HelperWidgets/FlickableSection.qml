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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Section {
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Flickable")

    SectionLayout {
        Label {
            text: qsTr("Content size")
        }

        SecondColumnLayout {

            Label {
                text: "W"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.contentWidth
                minimumValue: 0
                maximumValue: 8000
            }

            Label {
                text: "W"
                width: 12
            }

            SpinBox {
                backendValue: backendValues.contentHeight
                minimumValue: 0
                maximumValue: 8000

            }

            ExpandingSpacer {

            }
        }

        Label {
            text: qsTr("Flick direction")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.flickableDirection
                model: ["AutoFlickDirection", "HorizontalFlick", "VerticalFlick", "HorizontalAndVerticalFlick"]
                Layout.fillWidth: true
                scope: "Flickable"
            }

        }

        Label {
            text: qsTr("Behavior")
            tooltip: qsTr("Bounds behavior")
        }

        SecondColumnLayout {
            ComboBox {
                backendValue: backendValues.boundsBehavior
                model: ["StopAtBounds", "DragOverBounds", "DragAndOvershootBounds"]
                Layout.fillWidth: true
                scope: "Flickable"
            }

        }

        Label {
            text:qsTr("Interactive")
        }

        SecondColumnLayout {
            CheckBox {
                backendValue: backendValues.interactive
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Max. velocity")
            tooltip: qsTr("Maximum flick velocity")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.maximumFlickVelocity
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }

            ExpandingSpacer {
            }
        }

        Label {
            text: qsTr("Deceleration")
            tooltip: qsTr("Flick deceleration")
        }

        SecondColumnLayout {
            SpinBox {
                backendValue: backendValues.flickDeceleration
                minimumValue: 0
                maximumValue: 8000
                decimals: 0
            }

            ExpandingSpacer {
            }
        }

    }
}
