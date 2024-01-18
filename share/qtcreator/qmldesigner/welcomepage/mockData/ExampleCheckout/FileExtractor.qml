// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {

    property string targetPath
    property string archiveName
    property string detailedText
    property string currentFile
    property string size
    property string count
    property string sourceFile

    property bool finished
    property bool targetFolderExists

    property int progress
    property date birthTime
}
