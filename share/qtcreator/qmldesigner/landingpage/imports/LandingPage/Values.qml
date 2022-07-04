/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

pragma Singleton
import QtQuick 2.15

QtObject {
    id: values

    property string baseFont: "Titillium Web"

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
