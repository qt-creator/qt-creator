// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    readonly property alias hasMatchSearch: internal.matched
    readonly property alias hasDoneSearch: internal.searched

    property Item contentItem: null

    color: StudioTheme.Values.themeToolbarBackground
    height: StudioTheme.Values.toolbarHeight

    function clear() {
        internal.clear();
        searchBox.text = "";
        internal.timer.stop();
    }

    function search() {
        internal.search();
    }

    StudioControls.SearchBox {
        id: searchBox

        anchors.fill: parent
        anchors.bottomMargin: StudioTheme.Values.toolbarVerticalMargin
        anchors.topMargin: StudioTheme.Values.toolbarVerticalMargin
        anchors.leftMargin: StudioTheme.Values.toolbarHorizontalMargin
        anchors.rightMargin: StudioTheme.Values.toolbarHorizontalMargin
        style: StudioTheme.Values.searchControlStyle
        onSearchChanged: internal.timer.restart()
    }

    QtObject {
        id: internal

        readonly property var reverts: []
        readonly property Timer timer: Timer {
            interval: 300
            repeat: false

            onTriggered: internal.search()
        }
        property bool matched: false
        property bool searched: false

        function clear() {
            internal.reverts.forEach(revert => revert());
            internal.reverts.length = 0;
            internal.matched = false;
            internal.searched = false;
        }

        function search() {
            internal.clear();
            const searchText = searchBox.text.toLowerCase();
            if (searchText.length > 0) {
                internal.traverse(root.contentItem, searchText);
                internal.searched = true;
            }
        }

        function disableSearchNoMatchAction(item) {
            item.searchNoMatch = true;
            internal.reverts.push(() => {
                item.searchNoMatch = false;
            });
        }

        function disableVisibleAction(item) {
            item.visible = false;
            internal.reverts.push(() => {
                item.visible = true;
            });
        }

        function enableSearchHideAction(item) {
            item.searchHide = true;
            internal.reverts.push(() => {
                item.searchHide = false;
            });
        }

        function expandSectionAction(item) {
            internal.matched = true;
            const prevValue = item.expanded;
            item.expanded = true;
            internal.reverts.push(() => {
                item.expanded = prevValue;
            });
        }

        function traverse(item, searchText) {
            let hideSection = true;
            let hideParentSection = true;
            item.children.forEach((child, index, arr) => {
                if (!child.visible)
                    return;

                if (child.__isPropertyLabel) {
                    const propertyLabel = child;
                    const text = propertyLabel.text.toLowerCase();
                    if (!text.includes(searchText)) {
                        internal.disableSearchNoMatchAction(propertyLabel);
                        const action = propertyLabel.__inDynamicPropertiesSection ? internal.disableVisibleAction
                                                                                  : internal.disableSearchNoMatchAction;
                        const nextItem = arr[index + 1];
                        action(nextItem);
                    } else {
                        hideSection = false;
                    }
                }
                hideSection &= internal.traverse(child, searchText);
                if (child.__isSection) {
                    const action = hideSection ? internal.enableSearchHideAction
                                               : internal.expandSectionAction;
                    action(child);

                    if (child.__isInEffectsSection && !hideSection)
                        hideParentSection = false;

                    hideSection = true;
                }
            });
            return hideParentSection && hideSection;
        }
    }
}
