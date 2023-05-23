// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15

QtObject {
    id: values

    property Item mainScrollView

    property bool contextMenuOpened: false

    signal collapseAll(string category)
    signal expandAll(string category)
    signal closeContextMenu()
}
