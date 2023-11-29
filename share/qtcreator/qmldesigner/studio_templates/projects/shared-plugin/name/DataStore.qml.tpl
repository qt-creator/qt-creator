// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Singleton
import QtQuick 6.5
import QtQuick.Studio.Utils 1.0

JsonListModel {
    id: models
    source: Qt.resolvedUrl("models.json")

    property ChildListModel book: ChildListModel {
        modelName: "book"
    }

    property JsonData backend: JsonData {}
}
