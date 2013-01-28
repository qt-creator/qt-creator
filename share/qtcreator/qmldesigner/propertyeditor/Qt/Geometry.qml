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

GroupBox {
    finished: finishedNotify
    id: geometry

    caption: qsTr("Geometry")

    layout: VerticalLayout {
        bottomMargin: 12

        QWidget {  // 1
            layout: HorizontalLayout {

                Label {
                    text: qsTr("Position")
                }

                DoubleSpinBox {
                    id: xSpinBox;
                    text: "X"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    objectName: "xSpinBox";
                    enabled: anchorBackend.hasParent && !anchorBackend.leftAnchored && !anchorBackend.horizontalCentered
                    backendValue: backendValues.x
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    id: ySpinBox;
                    singleStep: 1;
                    text: "Y"
                    alignRight: false
                    spacing: 4
                    enabled: anchorBackend.hasParent && !anchorBackend.topAnchored && !anchorBackend.verticalCentered
                    backendValue: backendValues.y
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }


            }
        } //QWidget  //1

        QWidget {
            id: bottomWidget
            layout: HorizontalLayout {

                Label {
                    id: sizeLabel
                    text: qsTr("Size")
                }

                DoubleSpinBox {
                    toolTip: qsTr("Width")
                    id: widthSpinBox;
                    text: "W"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    enabled: !(anchorBackend.rightAnchored && anchorBackend.leftAnchored)
                    backendValue: backendValues.width
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }

                DoubleSpinBox {
                    toolTip: qsTr("Height")
                    id: heightSpinBox;
                    text: "H"
                    alignRight: false
                    spacing: 4
                    singleStep: 1;
                    enabled: !(anchorBackend.bottomAnchored && anchorBackend.topAnchored)
                    backendValue: backendValues.height
                    minimum: -2000;
                    maximum: 2000;
                    baseStateFlag: isBaseState;
                }
            } //QVBoxLayout
        } //QWidget
    } //QHBoxLayout
    
    QPushButton {
        checkable: true
        toolTip: qsTr("Lock aspect ratio")
        fixedWidth: 45
		width: fixedWidth
        fixedHeight: 9
		height: fixedHeight
        styleSheetFile: "aspectlock.css";
        x: bottomWidget.x + ((bottomWidget.width - sizeLabel.width) / 2) + sizeLabel.width - 8
        y: bottomWidget.y + bottomWidget.height + 2
        visible: false //hide until the visual editor implements this feature ###
    }
} //QGroupBox
