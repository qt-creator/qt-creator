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
