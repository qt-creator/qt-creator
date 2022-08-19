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

    padding.left: 6
    padding.right: 6
    padding.top: 1
    padding.bottom: style.style === "mac" ? 1 : style.style === "fusion" ? 3 : 2

    StyleItem { id: style ; visible: false}

    property Component panel: StyleItem {
        id: toolbar
        anchors.fill: parent
        elementType: "toolbar"
        textureWidth: 64
        border {left: 16 ; right: 16}
    }
}
