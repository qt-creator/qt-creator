// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Layouts 1.3

GridLayout {
    id: grid
    columns: 3

    Text { text: "Three"; font.bold: true; }
    Text { text: "words"; color: "red" }
    Text { text: "in"; font.underline: true }
    Text { text: "a"; font.pixelSize: 20 }
    Text { text: "row"; font.strikeout: true }
}
