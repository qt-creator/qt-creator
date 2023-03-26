// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick.Templates 2.15 as Controls

Controls.ScrollView {
    id: control

    Controls.ScrollBar.vertical: CustomScrollBar {
           parent: control
           x: control.mirrored ? 0 : control.width - width
           y: control.topPadding
           height: control.availableHeight
       }
}
