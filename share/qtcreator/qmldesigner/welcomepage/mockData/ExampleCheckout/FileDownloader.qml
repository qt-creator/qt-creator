// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    property url url
    property bool finished
    property bool error
    property string name
    property string completeBaseName
    property int progress
    property string outputFile
    property date lastModified
    property bool available
}
