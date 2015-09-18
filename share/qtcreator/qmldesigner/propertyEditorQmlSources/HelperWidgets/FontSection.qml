/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
