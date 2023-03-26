// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
