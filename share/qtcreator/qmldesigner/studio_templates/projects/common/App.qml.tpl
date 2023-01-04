// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick %{QtQuickVersion}
@if !%{IsQt6Project}
import QtQuick.Window %{QtQuickVersion}
@endif
import %{ImportModuleName} %{ImportModuleVersion}
@if %{UseVirtualKeyboard}
import QtQuick.VirtualKeyboard %{QtQuickVersion}
@endif

Window {
    width: mainScreen.width
    height: mainScreen.height

    visible: true
    title: "%{ProjectName}"

    %{UIClassName} {
        id: mainScreen
    }

@if %{UseVirtualKeyboard}
    InputPanel {
        id: inputPanel
        property bool showKeyboard :  active
        y: showKeyboard ? parent.height - height : parent.height
        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.InOutQuad
            }
        }
        anchors.leftMargin: Constants.width/10
        anchors.rightMargin: Constants.width/10
        anchors.left: parent.left
        anchors.right: parent.right
    }
@endif
}

