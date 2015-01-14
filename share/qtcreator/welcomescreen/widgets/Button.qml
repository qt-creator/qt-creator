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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Controls.Styles 1.0

Button {
    id: button
    style: touchStyle
    iconSource: ""
    Component {
        id: touchStyle
        ButtonStyle {
            padding.left: button.iconSource != "" ? 38 : 14
            padding.right: 14
            background: Item {
                anchors.fill: parent

                Image {
                    id: icon
                    z: 1
                    x: 4
                    y: -6
                    width: 32
                    height: 32
                    source: button.iconSource
                    visible: button.iconSource != ""
                }

                implicitWidth: 160
                implicitHeight: 30

                Rectangle {
                    anchors.fill: parent
                    antialiasing: true
                    radius: (creatorTheme.WidgetStyle === 'StyleFlat') ? 0 : 3

                    visible: !(button.pressed || button.checked)

                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#232323" : "#f9f9f9"
                        }

                        GradientStop {
                            position: 0.49
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#232323" : "#f9f9f9"
                        }

                        GradientStop {
                            position: 0.5
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#232323" : "#eeeeee"
                        }

                        GradientStop {
                            position: 1
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#232323" : "#eeeeee"
                        }
                    }
                    border.color: creatorTheme.Welcome_Button_BorderColorNormal
                }

                Rectangle {
                    anchors.fill: parent
                    antialiasing: true
                    radius: (creatorTheme.WidgetStyle === 'StyleFlat') ? 0 : 3

                    visible: button.pressed || button.checked

                    gradient: Gradient {
                        GradientStop {
                            position: 0.00;
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#151515" : "#4c4c4c"
                        }
                        GradientStop {
                            position: 0.49;
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#151515" : "#4c4c4c"
                        }
                        GradientStop {
                            position: 0.50;
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#151515" : "#424242"
                        }
                        GradientStop {
                            position: 1.00;
                            color: (creatorTheme.WidgetStyle === 'StyleFlat') ? "#151515" : "#424242"
                        }
                    }
                    border.color: creatorTheme.Welcome_Button_BorderColorPressed

                }
            }

            label: Text {
                    renderType: Text.NativeRendering
                    verticalAlignment: Text.AlignVCenter
                    text: button.text
                    color: button.pressed || button.checked
                             ? creatorTheme.Welcome_Button_TextColorPressed
                             : creatorTheme.Welcome_Button_TextColorNormal
                    font.pixelSize: 15
                    font.bold: false
                    smooth: true
                }
        }
    }
}
