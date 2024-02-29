// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import CollectionEditorBackend

ListView {
    id: root

    model: CollectionEditorBackend.model
    clip: true

    delegate: CollectionItem {
        implicitWidth: parent.width
        onDeleteItem: root.model.removeRow(index)
    }
}
