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

import QtQuick 2.4

/* This is a manual test for the dummy context described here:
http://blog.qt.io/blog/2011/07/28/mockup-data-for-the-qml-designer

The dummy context that defines parent and root is dummydata/context/main.qml.
Without the dummy context the root item does not have a defined size and the
color of the inner rectangle is not defined.

When Designer shows an error message about the missing parent,
click "Ignore" to see the rendering.

*/

Rectangle {
    width: parent.width
    height: parent.height

    Rectangle {
       anchors.centerIn: parent
        width: parent.width / 2
        height: parent.height / 2
        color: root.color
    }
}
