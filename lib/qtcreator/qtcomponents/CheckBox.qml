/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

import QtQuick 1.0
import "custom" as Components

// jb : Size should not depend on background, we should make it consistent

Components.CheckBox{
    id:checkbox
    property string text
    property string hint
    height:20
    width: Math.max(110, backgroundItem.textWidth(text) + 40)

    background: QStyleItem {
        elementType:"checkbox"
        sunken:pressed
        on:checked || pressed
        hover:containsMouse
        text:checkbox.text
        enabled:checkbox.enabled
        focus:checkbox.focus
        hint:checkbox.hint
    }
    Keys.onSpacePressed:checked = !checked
}

