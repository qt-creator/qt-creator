// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15
import StudioFonts 1.0

QtObject {
    id: values

    property string baseFont: StudioFonts.titilliumWeb_regular

    property real scaleFactor: 1.0
    property real checkBoxSize: Math.round(26 * values.scaleFactor)
    property real checkBoxIndicatorSize: Math.round(14 * values.scaleFactor)
    property real checkBoxSpacing: Math.round(6 * values.scaleFactor)
    property real border: 1

    property int fontSizeTitle: values.fontSizeTitleLG
    property int fontSizeSubtitle: values.fontSizeSubtitleLG

    readonly property int fontSizeTitleSM: 20
    readonly property int fontSizeTitleMD: 32
    readonly property int fontSizeTitleLG: 50

    readonly property int fontSizeSubtitleSM: 14
    readonly property int fontSizeSubtitleMD: 18
    readonly property int fontSizeSubtitleLG: 22

    // LG > 1000, MD <= 1000 && > 720, SM <= 720
    readonly property int layoutBreakpointLG: 1000
    readonly property int layoutBreakpointMD: 720

    readonly property int spacing: 20
}
