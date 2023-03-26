// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

TimelineText {
    property bool isLabel: false
    property int valueWidth: 170
    property int labelWidth: implicitWidth
    font.bold: isLabel
    elide: isLabel ? Text.ElideNone : Text.ElideRight
    width: isLabel ? labelWidth : valueWidth
}
