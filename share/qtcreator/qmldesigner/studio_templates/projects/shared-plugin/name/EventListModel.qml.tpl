// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

ListModel {
    id: eventListModel

    ListElement {
        eventId: "enterPressed"
        eventDescription: "Emitted when pressing the enter button"
        shortcut: "Return"
        parameters: "Enter"
    }
}
