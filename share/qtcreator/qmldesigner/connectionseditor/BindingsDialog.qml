// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

PopupDialog {
    property alias backend: form.backend

    BindingsDialogForm {
        id: form
        y: 32
    }
}
