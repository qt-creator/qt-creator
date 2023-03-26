// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import StudioFonts 1.0
import projectmodel 1.0
import usagestatistics 1.0

EllipseItem {
    id: ellipse
    width: 529
    height: 391
    opacity: 0.495
    layer.enabled: true
    layer.effect: FastBlurEffect {
        id: fastBlur
        radius: 66
        transparentBorder: true
        cached: true
    }
    fillColor: "#878787"
    strokeColor: "#00ff0000"
}
