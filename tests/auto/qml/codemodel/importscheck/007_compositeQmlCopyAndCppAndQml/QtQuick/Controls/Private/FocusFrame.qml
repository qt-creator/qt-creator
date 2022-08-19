// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype FocusFrame
    \internal
    \inqmlmodule QtQuick.Controls.Private
*/
Item {
    id: root
    activeFocusOnTab: false
    Accessible.role: Accessible.StatusBar

    anchors.topMargin: focusMargin
    anchors.leftMargin: focusMargin
    anchors.rightMargin: focusMargin
    anchors.bottomMargin: focusMargin

    property int focusMargin: loader.item ? loader.item.margin : -3

    Loader {
        id: loader
        z: 2
        anchors.fill: parent
        sourceComponent: Qt.createComponent(Settings.style + "/FocusFrameStyle.qml", root)
    }
}
