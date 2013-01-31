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
import "custom" as Components

Components.TextField {
    id: textfield
    minimumWidth: 200

    placeholderText: ""
    topMargin: 2
    bottomMargin: 2
    leftMargin: 6
    rightMargin: 6

    property string hint

    height:  backgroundItem.sizeFromContents(200, 25).height
    width: 200
    clip: false

    background: QStyleItem {
        anchors.fill: parent
        elementType: "edit"
        sunken: true
        focus: textfield.activeFocus
        hover: containsMouse
        hint: textfield.hint
    }

    Item{
        id: focusFrame
        anchors.fill: textfield
        parent: textfield
        visible: framestyle.styleHint("focuswidget")
        QStyleItem {
            id: framestyle
            anchors.margins: -2
            anchors.rightMargin:-4
            anchors.bottomMargin:-4
            anchors.fill: parent
            visible: textfield.activeFocus
            elementType: "focusframe"
        }
    }
}
