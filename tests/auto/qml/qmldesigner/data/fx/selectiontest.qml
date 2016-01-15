/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import Qt 4.7

Item {
    id: rootItem
    width: 640
    height: 480
Rectangle {
        height: 100;
        x: 104;
        y: 178;
        color: "#ffffff";
        width: 100;
        id: rectangle_1;
    }
Rectangle {
        x: 110;
        y: 44;
        color: "#ffffff";
        height: 100;
        width: 100;
        id: rectangle_2;
    }
Rectangle {
        width: 100;
        color: "#ffffff";
        height: 100;
        x: 323;
        y: 160;
        id: rectangle_3;
    }
Rectangle {
        color: "#ffffff";
        width: 100;
        height: 100;
        x: 233;
        y: 293;
        id: rectangle_4;
    }
}
