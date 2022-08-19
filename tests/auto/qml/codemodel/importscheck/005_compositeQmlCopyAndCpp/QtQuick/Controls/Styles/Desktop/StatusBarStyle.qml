// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype StatusBarStyle
    \internal
    \inqmlmodule QtQuick.Controls.Styles
*/
Style {

    padding.left: 4
    padding.right: 4
    padding.top: 3
    padding.bottom: 2

    property Component panel: StyleItem {
        implicitHeight: 16
        implicitWidth: 200
        anchors.fill: parent
        elementType: "statusbar"
        textureWidth: 64
        border {left: 16 ; right: 16}
    }
}
