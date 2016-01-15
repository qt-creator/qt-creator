/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
