// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import HelperWidgets as HelperWidgets
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
                internal.traverse(root.contentItem, searchText, false);
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

        function traverse(item, searchText, isInSection) {
            let hideSection = true;
            let hideParentSection = true;
            item.children.forEach((child, index, arr) => {
                if (!child.visible)
                    return;

                if (child instanceof HelperWidgets.PropertyLabel) {
                    const propertyLabel = child;
                    const text = propertyLabel.text.toLowerCase();
                    // Some complex properties include empty labels for layout purposes, ignore those
                    if (text !== "") {
                        if (!text.includes(searchText)) {
                            internal.disableSearchNoMatchAction(propertyLabel);
                            let nextIndex = index;
                            let nextItem = arr[++nextIndex];
                            if (nextItem instanceof HelperWidgets.SecondColumnLayout)
                                internal.disableSearchNoMatchAction(nextItem)
                            else
                                internal.disableVisibleAction(nextItem)

                            while (nextIndex < arr.length - 1) {
                                nextItem = arr[++nextIndex];
                                if ((nextItem instanceof HelperWidgets.PropertyLabel && nextItem.text !== "")
                                    || nextItem instanceof HelperWidgets.Section) {
                                    break;
                                }
                                if (nextItem instanceof HelperWidgets.SecondColumnLayout
                                    || nextItem instanceof HelperWidgets.PropertyLabel) {
                                    internal.disableSearchNoMatchAction(nextItem);
                                }
                            }
                        } else {
                            hideSection = false;
                        }
                    }
                }
                hideSection &= internal.traverse(child, searchText,
                                                 isInSection || child instanceof HelperWidgets.Section);
                if (child instanceof HelperWidgets.Section) {
                    const action = hideSection ? internal.enableSearchHideAction
                                               : internal.expandSectionAction;
                    action(child);

                    if (isInSection && !hideSection)
                        hideParentSection = false;

                    hideSection = true;
                }
            });
            return hideParentSection && hideSection;
        }
    }
}
