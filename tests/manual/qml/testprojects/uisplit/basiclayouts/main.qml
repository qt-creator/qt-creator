// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    title: "Basic layouts"
    property int margin: 11
    width: mainForm.implicitWidth + 2 * margin
    height: mainForm.implicitHeight + 2 * margin
    minimumWidth: mainForm.Layout.minimumWidth + 2 * margin
    minimumHeight: mainForm.Layout.minimumHeight + 2 * margin

    MainForm {
        id: mainForm
        anchors.margins: margin
    }

}
