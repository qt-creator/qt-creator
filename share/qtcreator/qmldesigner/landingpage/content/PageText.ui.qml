// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import LandingPage as Theme

Text {
    font.family: Theme.Values.baseFont
    font.pixelSize: Theme.Values.fontSizeSubtitle
    horizontalAlignment: Text.AlignHCenter
    color: Theme.Colors.text
    anchors.horizontalCenter: parent.horizontalCenter
    wrapMode: Text.WordWrap
}
