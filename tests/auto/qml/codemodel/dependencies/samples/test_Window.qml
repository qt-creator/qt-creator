// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// ExpectedSemanticMessages: 0
// ExpectedStaticMessages: 0

import QtQuick 2.4
import QtQuick.Window 2.2

Window {
    title: qsTr("Hello World")
    width: 640
    height: 480
    visible: true
}
