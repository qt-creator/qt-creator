// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Layouts 1.0

Row {
    id: buttonRow

    property bool exclusive: false
    property int initalChecked: 0
    property int checkedIndex: 0

    signal toggled (int index, bool checked)
}
