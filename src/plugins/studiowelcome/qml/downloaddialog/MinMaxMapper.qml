// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.10

QtObject {
    id: object


    property real input

    property bool minClipped: object.input < object.min
    property bool maxClipped: object.input > object.max
    property bool outOfRage: object.maxClipped ||object.minClipped

    property real value: {
        if (object.maxClipped)
            return object.max

        if (object.minClipped)
            return object.min

        return object.input
    }

    property real min: 0
    property real max: 100
}
