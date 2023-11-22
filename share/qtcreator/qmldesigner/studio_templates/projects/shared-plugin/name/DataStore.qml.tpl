// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Singleton
import QtQuick.Studio.Utils

JsonListModel {
    property alias allModels: models
    id: models

    source: Qt.resolvedUrl("DataStore.json")
}
