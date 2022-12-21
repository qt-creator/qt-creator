// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

QtObject {
    signal finished

    property string sourceFile: "SomeExample.zip"
    property string archiveName: "SomeExample"

    property string targetPath: "/extract/here"

    property string detailedText: "Start" +
                                  "\n1 Some detailed info about extraction\nSome detailed info about extraction\nSome detailed info about extractionSome detailed info about extractionSome detailed info about extraction" +
                                  "\n2 Some detailed info about extraction\nSome detailed info about extraction\nSome detailed info about extractionSome detailed info about extractionSome detailed info about extraction" +
                                  "\n3 Some detailed info about extraction\nSome detailed info about extraction\nSome detailed info about extractionSome detailed info about extractionSome detailed info about extraction" +
                                  "\nend"
}
