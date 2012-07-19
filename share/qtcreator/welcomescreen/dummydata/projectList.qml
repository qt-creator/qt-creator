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

ListModel {
    ListElement {
        prettyFilePath: "showing some path aka the link..."
        displayName: "Project name 01"
    }
    ListElement {
        prettyFilePath: "showing some path aka link..."
        displayName: "Project name 02"
    }
    ListElement {
        prettyFilePath: "showing some ... path aka link..."
        displayName: "Project name 03"
    }
    ListElement {
        prettyFilePath: "showing some ... path aka link..."
        displayName: "Project name 04"
    }
    ListElement {
        prettyFilePath: "showing some ... path aka link..."
        displayName: "Project name 05"
    }
    ListElement {
        prettyFilePath: "showing some ... path aka link test..."
        displayName: "Project name 06"
    }
    ListElement {
        prettyFilePath: "showing some ... path aka link... blup"
        displayName: "Project name 07"
    }
    ListElement {
        prettyFilePath: "showing some ...  bla path aka link..."
        displayName: "Project name 08"
    }
}
