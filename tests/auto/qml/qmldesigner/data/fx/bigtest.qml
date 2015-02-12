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

import Qt 4.7

Item {
    width: 640;
    height: 480;
    Rectangle {
        width: 295;
        height: 264;
        color: "#009920";
        x: 22;
        y: 31;
        id: rectangle_1;
        Rectangle {
            width: 299;
            color: "#009920";
            x: 109;
            height: 334;
            y: 42;
            id: rectangle_3;
            Rectangle {
                color: "#009920";
                height: 209;
                width: 288;
                x: 98;
                y: 152;
                id: rectangle_5;
            }
            
            
        Image {
                source: "qrc:/fxplugin/images/template_image.png";
                height: 379;
                x: -154;
                width: 330;
                y: -31;
                id: image_1;
            }Image {
                width: 293;
                x: 33;
                height: 172;
                y: 39;
                source: "qrc:/fxplugin/images/template_image.png";
                id: image_2;
            }}
        
    }
    Rectangle {
        width: 238;
        height: 302;
        x: 360;
        y: 42;
        color: "#009920";
        id: rectangle_2;
    Image {
            source: "qrc:/fxplugin/images/template_image.png";
            x: -40;
            y: 12;
            width: 281;
            height: 280;
            id: image_3;
        }}
    Rectangle {
        height: 206;
        width: 276;
        color: "#009920";
        x: 4;
        y: 234;
        id: rectangle_4;
    }
    Text {
        text: "text";
        width: 80;
        height: 20;
        x: 71;
        y: 15;
        id: text_1;
    }
}
