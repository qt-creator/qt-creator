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

import QtQuick 2.0

Rectangle {
    id: rect
    width: 200
    height: 200
    Image {
        id: image1
        x: 41
        y: 46
        source: "images/qtcreator.png"
    }
    Text {
        id: textItem
        x: 66
        y: 93
        text: "Base State"
    }
    states: [
        State {
            name: "State1"
            PropertyChanges {
                target: rect
                color: "blue"
            }
            PropertyChanges {
                target: textItem
                text: "State1"
            }
        },
        State {
            name: "State2"
            PropertyChanges {
                target: rect
                color: "gray"
            }
            PropertyChanges {
                target: textItem
                text: "State2"
            }
        }
    ]
}
