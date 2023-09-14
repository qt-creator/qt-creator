// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma Singleton
import QtQuick 2.15

QtObject {
    id: root

    // counts of sections in each category, allows accessing the count from inside a section
    property var counts: ({})

    property Item mainScrollView

    property bool contextMenuOpened: false

    function count(category) {
        if (!root.counts.hasOwnProperty(category))
            return 0

        return root.counts[category]
    }

    function setCount(category, count) {
        root.counts[category] = count
        root.countChanged(category, count)
    }

    signal collapseAll(string category)
    signal expandAll(string category)
    signal closeContextMenu()
    signal countChanged(string category, int count)
}
