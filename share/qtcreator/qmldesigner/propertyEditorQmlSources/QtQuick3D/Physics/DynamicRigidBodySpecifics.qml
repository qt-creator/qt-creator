// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets

Column {
    width: parent.width

    DynamicRigidBodySection {
        width: parent.width
    }

    PhysicsBodySection {
        width: parent.width
    }

    PhysicsNodeSection {
        width: parent.width
    }

    NodeSection {
        width: parent.width
    }
}
