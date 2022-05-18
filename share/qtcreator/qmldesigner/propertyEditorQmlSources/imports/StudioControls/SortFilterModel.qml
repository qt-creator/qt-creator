/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQml.Models

DelegateModel {
    id: delegateModel

    property var visibleGroup: visibleItems

    property var lessThan: function(left, right) { return true }
    property var filterAcceptsItem: function(item) { return true }

    signal updated()

    function update() {
        if (delegateModel.items.count > 0) {
            delegateModel.items.setGroups(0, delegateModel.items.count, "items")
        }

        // Filter items
        var visible = []
        for (var i = 0; i < delegateModel.items.count; ++i) {
            var item = delegateModel.items.get(i)
            if (delegateModel.filterAcceptsItem(item.model)) {
                visible.push(item)
            }
        }

        // Sort the list of visible items
        visible.sort(function(a, b) {
            return delegateModel.lessThan(a.model, b.model);
        });

        // Add all items to the visible group
        for (i = 0; i < visible.length; ++i) {
            item = visible[i]
            item.inVisible = true
            if (item.visibleIndex !== i) {
                visibleItems.move(item.visibleIndex, i, 1)
            }
        }

        delegateModel.updated()
    }

    items.onChanged: delegateModel.update()
    onLessThanChanged: delegateModel.update()
    onFilterAcceptsItemChanged: delegateModel.update()

    groups: DelegateModelGroup {
        id: visibleItems

        name: "visible"
        includeByDefault: false
    }

    filterOnGroup: "visible"
}
