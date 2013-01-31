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

QGroupBox {
    id: fontStyleButtons

    property int buttonWidth: 46

    property variant bold: backendValues.font_bold
    property variant italic: backendValues.font_italic
    property variant underline: backendValues.font_underline
    property variant strikeout: backendValues.font_strikeout

    layout: HorizontalLayout {

        QWidget {
            fixedHeight: 32

            FlagedButton {
                checkable: true
                iconSize.width: 24;
                iconSize.height: 24;
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                height: fixedHeight
                styleSheetFile: "styledbuttonleft.css";
                checked: bold.value;
                backendValue: bold;

                iconFromFile: flagActive ? "images/bold-h-icon.png" : "images/bold-icon.png"

                onClicked: {
                    bold.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue: bold
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile: flagActive ? "images/italic-h-icon.png" : "images/italic-icon.png"

                styleSheetFile: "styledbuttonmiddle.css";
                checked: italic.value;
                backendValue: italic;

                onClicked: {
                    italic.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue: italic
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth * 2
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile:  flagActive ? "images/underline-h-icon.png" : "images/underline-icon.png"

                styleSheetFile: "styledbuttonmiddle.css";
                checked: underline.value;
                backendValue: underline;

                onClicked: {
                    underline.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue: underline;
                    y: 7
                    x: 2
                }
            }
            FlagedButton {
                x: buttonWidth * 3
                checkable: true
                fixedWidth: buttonWidth
                width: fixedWidth
                fixedHeight: 28
                iconSize.width: 24;
                iconSize.height: 24;
                height: fixedHeight
                iconFromFile: flagActive ? "images/strikeout-h-icon.png" : "images/strikeout-icon.png"

                styleSheetFile: "styledbuttonright.css";
                checked: strikeout.value;
                backendValue: strikeout;

                onClicked: {
                    strikeout.value = checked;
                }

                ExtendedFunctionButton {
                    backendValue: strikeout;
                    y: 7
                    x: 2
                }
            }
        }
    }
}
