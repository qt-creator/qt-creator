// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0

Item {
    id: root
    width: 150
    height: 150

    property alias contentItem: contentItem

    /*
    View3D {
        // Dummy view to hold the context in case View3D items are used in the component
        // TODO remove when QTBUG-87678 is fixed
    }
    */

    Item {
        id: contentItem
    }
}
