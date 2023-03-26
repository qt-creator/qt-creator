// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.10
import QtQuick.Window 2.10

Window {
    visible: true
    width: 180
    height: 215

    PixelMeter {
        width: 60
        height: 40
        x: 0
        y: 0
    }

    PixelMeter {
        x: 60
        y: 40
        width: 120
        height: 175

        PixelMeter {
            x: 80
            y: 40
            width: 40
            height: 40
        }

        PixelMeter {
            x: 80
            y: 0
            width: 40
            height: 40
        }

        PixelMeter {
            x: 40
            y: 0
            width: 40
            height: 40
        }

        PixelMeter {
            x: 40
            y: 40
            width: 40
            height: 40
        }
    }
}
