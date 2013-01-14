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

import QtQuick 2.0

Rectangle {
    width: 232
    height: 232

    Column {
        id: column
        x: 39
        y: 20
        spacing: 2

        Rectangle {
            width: 20
            height: 20
            color: "#c2d11b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#d11b1b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#1e3fd3"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#3bd527"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8726b7"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8b8b8b"
        }
    }

    Row {
        id: row
        x: 78
        y: 20
        Rectangle {
            width: 20
            height: 20
            color: "#c2d11b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#d11b1b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#1e3fd3"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#3bd527"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8726b7"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8b8b8b"
        }
        spacing: 2
    }

    Flow {
        id: flow
        x: 78
        y: 53
        width: 84
        height: 31

        Rectangle {
            width: 20
            height: 20
            color: "#c2d11b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#d11b1b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#1e3fd3"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#3bd527"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8726b7"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8b8b8b"
        }
        spacing: 2
    }

    Grid {
        id: grid
        x: 78
        y: 108
        columns: 3
        Rectangle {
            width: 20
            height: 20
            color: "#c2d11b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#d11b1b"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#1e3fd3"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#3bd527"
        }

        Rectangle {
            width: 20
            height: 20
            color: "#8726b7"
        }

        spacing: 2
        Rectangle {
            width: 20
            height: 20
            color: "#8b8b8b"
        }
    }

}
