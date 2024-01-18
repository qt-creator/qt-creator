// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

QtObject {
    id: object

/*!
    The input value.
*/
    property real input: 0

/*!
    The output value.
*/
    property real output: {
        var slope = (object.outputMaximum - object.outputMinimum) / (object.inputMaximum - object.inputMinimum)
        return object.outputMinimum + slope * (object.input - object.inputMinimum)
    }

/*!
    The minimum input value.
*/
    property real inputMinimum: 0

/*!
    The maximum input value.
*/
    property real inputMaximum: 100

/*!
    The minimum output value.
*/
    property real outputMinimum: 0

/*!
    The maximum output value.
*/
    property real outputMaximum: 100

}
