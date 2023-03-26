// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

BorderImage{
    source: "../images/header.png"
    border{left:2;right:2;top:2;bottom:2}
    Text {
        text: styleData.value
        anchors.centerIn:parent
        color:"#333"
    }
}
