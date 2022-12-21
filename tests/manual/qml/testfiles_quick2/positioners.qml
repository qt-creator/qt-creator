// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        id: flowPositioner
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
