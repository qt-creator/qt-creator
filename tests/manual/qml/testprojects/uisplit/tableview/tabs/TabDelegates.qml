// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

import "../delegates"

TabDelegatesForm {
    anchors.fill: parent

    tableView.model: largeModel
    tableView.frameVisible: frameCheckbox.checked
    tableView.headerVisible: headerCheckbox.checked
    tableView.sortIndicatorVisible: sortableCheckbox.checked
    tableView.alternatingRowColors: alternateCheckbox.checked

    tableView.itemDelegate: {
        if (delegateChooser.currentIndex == 2)
            return editableDelegate;
        else
            return delegate1;
    }

    EditableDelegate {
        id: editableDelegate
    }

    Delegate1 {
        id: delegate1
    }

    Delegate2 {
        id: delegate2
    }

}
