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
import QtQuick.Controls 1.0 as Controls

Section {
    id: fontSection
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr("Font")

    property bool showStyle: false

    property variant fontFamily: backendValues.font_family
    property variant pointSize: backendValues.font_pointSize
    property variant pixelSize: backendValues.font_pixelSize

    property variant boldStyle: backendValues.font_bold
    property variant italicStyle: backendValues.font_italic
    property variant underlineStyle: backendValues.font_underline
    property variant strikeoutStyle: backendValues.font_strikeout

    onPointSizeChanged: {
        sizeWidget.setPointPixelSize();
    }

    onPixelSizeChanged: {
        sizeWidget.setPointPixelSize();
    }


    SectionLayout {
        columns: 2
        Label {
            text: qsTr("Font")
        }
        FontComboBox {
            backendValue: fontFamily
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Size")
        }

        RowLayout {
            id: sizeWidget
            property bool selectionFlag: selectionChanged

            property bool pixelSize: sizeType.currentText === "pixels"
            property bool isSetup;


            function setPointPixelSize() {
                sizeWidget.isSetup = true;
                sizeType.currentIndex = 1
                if (fontSection.pixelSize.isInModel)
                    sizeType.currentIndex = 0
                sizeWidget.isSetup = false;
            }

            onSelectionFlagChanged: {
                sizeWidget.setPointPixelSize();
            }

            Item {
                width: sizeSpinBox.width
                height: sizeSpinBox.height

                SpinBox {
                    id: sizeSpinBox
                    minimumValue: 0
                    //visible: !sizeWidget.pixelSize
                    z: !sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pointSize
                }

                SpinBox {
                    minimumValue: 0
                    //visible: sizeWidget.pixelSize
                    z: sizeWidget.pixelSize ? 1 : 0
                    maximumValue: 400
                    backendValue: pixelSize
                }
            }

            Controls.ComboBox {
                id: sizeType
                model: ["pixels", "points"]
                onCurrentIndexChanged: {
                    if (sizeWidget.isSetup)
                        return;
                    if (currentText == "pixels") {
                        pointSize.resetValue();
                        pixelSize.value = 8;
                    } else {
                        pixelSize.resetValue();
                    }

                }

                Layout.fillWidth: true

                style: CustomComboBoxStyle {
                }

            }

        }

        Label {
            text: qsTr("Font style")
        }
        FontStyleButtons {

        }

        Label {
            visible: showStyle
            text: qsTr("Style")
        }

        ComboBox {
            visible: showStyle
            Layout.fillWidth: true
            backendValue: (backendValues.style === undefined) ? dummyBackendValue : backendValues.style
            model:  ["Normal", "Outline", "Raised", "Sunken"]
            scope: "Text"
        }
    }
}
