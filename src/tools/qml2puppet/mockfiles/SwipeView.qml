// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.6
import QtQuick.Layouts 1.3

StackLayout {
    id: root
    //property alias index: root.currentIndex
    property bool interactive: true
    default property alias contentData: root.data
}
