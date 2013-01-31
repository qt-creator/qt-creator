/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import Bauhaus 1.0
import HelperWidgets 1.0

GroupBox {
    id: rectangleColorGroupBox

    caption: qsTr("Colors")


    property bool selectionFlag: selectionChanged

    property bool isSetup;

    property bool hasBorder

    property variant colorAlpha: colorGroupBox.alpha
    property bool hasGradient: backendValues.gradient.isInModel

    property bool gradientIsBinding: backendValues.gradient.isBound

    onHasGradientChanged: {
        print("onGradientInModelChanged")
        if (backendValues.gradient.isInModel) {
            print("inmodel")
            colorGroupBox.setGradientButtonChecked = true;
        } else {
            print("else")
            if (colorGroupBox.alpha == 0)
                colorGroupBox.setNoneButtonChecked = true;
            else
                colorGroupBox.setSolidButtonChecked = true;
        }
    }

    onColorAlphaChanged: {
          if (backendValues.gradient.isInModel)
              return
        if (colorGroupBox.alpha == 0)
            colorGroupBox.setNoneButtonChecked = true;
        else
            colorGroupBox.setSolidButtonChecked = true;
    }

    onSelectionFlagChanged: {
        isSetup = true;
        gradientLine.active = false;
        colorGroupBox.setSolidButtonChecked = true;
        if (backendValues.gradient.isInModel && !gradientIsBinding) {
            colorGroupBox.setGradientButtonChecked = true;
            gradientLine.active = true;
            gradientLine.setupGradient();
        }
        if (colorGroupBox.alpha == 0) {
            colorGroupBox.setNoneButtonChecked = true;
            //borderColorBox.collapseBox = true
        }

        if (backendValues.border_color.isInModel || backendValues.border_width.isInModel) {
            borderColorBox.setSolidButtonChecked = true;
            hasBorder = true;
        } else {
            borderColorBox.setNoneButtonChecked = true;
            hasBorder = false
            //borderColorBox.collapseBox = true
        }

        isSetup = false
    }

    layout: VerticalLayout {

        QWidget {
            visible: colorGroupBox.gradientButtonChecked
            layout: HorizontalLayout {
                spacing: 2
                Label {
                    text: qsTr("Stops")
                    toolTip: qsTr("Gradient stops")
                }

                GradientLine {
                    id: gradientLine;
                    activeColor: colorGroupBox.color
                    itemNode: anchorBackend.itemNode
                }
            }
        }

        ColorGroupBox {
            enabled: !gradientIsBinding
            opacity: gradientIsBinding ? 0.7 : 1
            id: colorGroupBox
            caption: qsTr("Rectangle")
            finished: finishedNotify
            backendColor: backendValues.color
            //gradientEditing: true;
            gradientColor: gradientLine.activeColor;
            showButtons: true;
            showGradientButton: true;

            onGradientButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (gradientButtonChecked) {
                    gradientLine.active = true
                    gradientLine.setupGradient();
                    backendValues.color.resetValue();
                } else {
                    gradientLine.active = false
                    if (backendValues.gradient.isInModel)
                        gradientLine.deleteGradient();

                }
            }

            onNoneButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (noneButtonChecked) {
                    backendValues.color.value = "transparent";
                    collapseBox = false
                } else {
                    alpha = 255;
                }
            }

            onSolidButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (solidButtonChecked) {
                    backendValues.color.resetValue();
                }
            }
        }

        ColorGroupBox {
            id: borderColorBox
            caption: qsTr("Border")
            finished: finishedNotify
            showButtons: true;

            backendColor: backendValues.border_color

            property bool backendColorValue: backendValues.border_color.isInModel
            enabled: isBaseState || hasBorder

            onBackendColorValueChanged: {
                if (backendValues.border_color.isInModel) {
                    borderColorBox.setSolidButtonChecked = true;
                    hasBorder = true;
                } else {
                    borderColorBox.setNoneButtonChecked = true;
                    hasBorder = false;
                }
            }

            onNoneButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (noneButtonChecked) {
                    transaction.start();
                    backendValues.border_color.resetValue();
                    backendValues.border_width.resetValue();
                    transaction.end();
                    hasBorder = false
                    collapseBox = false
                } else {
                    transaction.start();
                    backendValues.border_color.value = "blue"
                    backendValues.border_color.value = "#000000";
                    transaction.end();
                    hasBorder = true
                }
            }

            onSolidButtonCheckedChanged: {
                if (isSetup)
                    return;
                if (solidButtonChecked) {
                    transaction.start();
                    backendValues.border_color.value = "blue"
                    backendValues.border_color.value = "#000000";
                    transaction.end();
                    hasBorder = true
                }
            }
        }

    }

}
