// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.10

QtObject {
    id: object


    property real input

    property real value: {
        var slope = (object.outputMax - object.outputMin) / (object.inputMax - object.inputMin)
        return object.outputMin + slope * (object.input - object.inputMin)
    }

    property real inputMin: 0
    property real inputMax: 100
    property real outputMin: 0
    property real outputMax: 100

}
