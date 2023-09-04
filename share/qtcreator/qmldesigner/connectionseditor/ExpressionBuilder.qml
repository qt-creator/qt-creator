// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme

Rectangle {
    id: root

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    property var conditionListModel: ConnectionsEditorEditorBackend.connectionModel.delegate.conditionListModel

    property alias model: repeater.model
    property int shadowPillIndex: -1
    property bool shadowPillVisible: root.shadowPillIndex !== -1

    property int heightBeforeShadowPill: Math.min(20, flow.childrenRect.height) // TODO Proper size value
    property int expressionHeight: {
        if (popup.visible)
            return root.heightBeforeShadowPill + flow.spacing + 20

        return root.heightBeforeShadowPill
    }

    signal remove(int index)
    signal update(int index, var value)
    signal add(var value)
    signal insert(int index, var value, int type)

    signal setValue(int index, var value)
    signal setValueType(int index, var value, int type)

    width: 400
    height: root.expressionHeight + 2 * StudioTheme.Values.flowMargin
    color: root.style.background.idle
    border {
        color: root.conditionListModel.valid ? root.style.border.idle
                                             : StudioTheme.Values.themeError
        width: root.style.borderWidth
    }

    onVisibleChanged: {
        if (!root.visible)
            popup.close()
    }

    // Is text input for creating new items currently used.
    function textInputActive() { // TODO Make property
        return newTextInput.activeFocus && newTextInput.visible
    }

    function getMappedItemRect(index: int) : rect {
        let item = repeater.itemAt(index)
        let itemRect = Qt.rect(item.x, item.y, item.width, item.height)
        return flow.mapToItem(root, itemRect)
    }

    function placeCursor(index: int) : void {
        var textInputPosition = Qt.point(0, 0)

        if (!repeater.count) { // Empty repeater
            let mappedItemRect = flow.mapToItem(root, 0, 0, 0, 0)

            textInputPosition = Qt.point(mappedItemRect.x, mappedItemRect.y)
            index = 0
        } else { // Repeater is not empty
            // Clamp index to 0 and num items in repeater
            index = Math.min(Math.max(index, 0), repeater.count)

            if (index === 0) {
                // Needs to be placed in front of first repeater item
                let mappedItemRect = root.getMappedItemRect(index)
                textInputPosition = Qt.point(mappedItemRect.x - 4, // - 4 due to spacing of flow
                                             mappedItemRect.y)
            } else {
                let mappedItemRect = root.getMappedItemRect(index - 1)
                textInputPosition = Qt.point(mappedItemRect.x + mappedItemRect.width + 3,
                                             mappedItemRect.y)
            }
        }

        // Position text input, make it visible and set focus
        newTextInput.x = textInputPosition.x
        newTextInput.y = textInputPosition.y
        newTextInput.index = index
        newTextInput.visible = true
        newTextInput.forceActiveFocus()
        // Open suggestion popup
        popup.open()
    }

    StudioControls.ToolTip {
        id: toolTip
        visible: mouseArea.containsMouse && toolTip.text !== ""
        delay: 1000
        text: root.conditionListModel.error
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.IBeamCursor
        hoverEnabled: true

        onPressed: function (event) {
            // Check if empty
            if (!repeater.count) {
                root.placeCursor(0)
                return
            }

            // Map to flow item
            let point = mouseArea.mapToItem(flow, Qt.point(event.x, event.y))

            let horizontalDistance = Number.MAX_VALUE
            let verticalDistance = Number.MAX_VALUE
            let cursorPosition = 0

            for (var i = 0; i < repeater.count; ++i) {
                let item = repeater.itemAt(i)

                let y = item.y + (item.height / 2)

                // Vertical distance
                let vDistance = Math.abs(point.y - y)

                // Horizontal distance
                let hLeftDistance = Math.abs(point.x - item.x)
                let hRightDistance = Math.abs(point.x - (item.x + item.width))

                // Early return if vertical distance increases
                if (vDistance > verticalDistance)
                    break

                if (vDistance <= verticalDistance) {
                    // Rest horizontal distance if vertical distance is smaller than before
                    if (vDistance !== verticalDistance)
                        horizontalDistance = Number.MAX_VALUE

                    if (hLeftDistance < horizontalDistance) {
                        horizontalDistance = hLeftDistance
                        cursorPosition = i
                    }

                    if (hRightDistance < horizontalDistance) {
                        horizontalDistance = hRightDistance
                        cursorPosition = i + 1
                    }

                    verticalDistance = vDistance
                }
            }

            root.placeCursor(cursorPosition)
        }
    }

    Flow {
        id: flow

        property int focusIndex: -1

        anchors.fill: parent
        anchors.margins: StudioTheme.Values.flowMargin
        spacing: StudioTheme.Values.flowSpacing

        onPositioningComplete: {
            if (root.textInputActive())
                root.placeCursor(newTextInput.index)

            if (!root.shadowPillVisible)
                root.heightBeforeShadowPill = flow.childrenRect.height
        }

        Repeater {
            id: repeater

            onItemRemoved: function(index, item) {
                if (!root.textInputActive())
                    return

                // Udpate the cursor position
                if (index < newTextInput.index)
                    newTextInput.index = newTextInput.index - 1
            }

            onItemAdded: function(index, item) {
                if (!root.textInputActive())
                    return

                if (index >= newTextInput.index)
                    newTextInput.index = newTextInput.index + 1

                if (!root.conditionListModel.valid && index === root.conditionListModel.errorIndex)
                    item.invalid = true
            }

            Pill {
                id: pill

                onRemove: function() {
                    // If pill has focus due to selection or keyboard navigation
                    if (pill.focus)
                        root.placeCursor(pill.index)

                    Qt.callLater(root.remove, pill.index)
                }

                onUpdate: function(value) {
                    if (value === "")
                        Qt.callLater(root.remove, pill.index) // Otherwise crash
                    else
                        Qt.callLater(root.update, pill.index, value)
                }

                onFocusChanged: function() {
                    if (pill.focus)
                        flow.focusIndex = pill.index
                }

                onSubmit: {
                    console.log("SUBMIT")

                    //newTextInput.index = pill.index + 1
                    newTextInput.visible = true
                    newTextInput.forceActiveFocus()
                }
            }
        }
    }

    TextInput {
        id: newTextInput

        property int index

        height: 20
        topPadding: 1
        font.pixelSize: root.style.baseFontSize
        color: root.style.text.idle
        visible: false
        validator: RegularExpressionValidator { regularExpression: /^\S.+/ }

        //onActiveFocusChanged: {
        //    if (!newTextInput.activeFocus && !root.shadowPillVisible) {
        //        console.log("CLOSE POPUP")
        //        popup.close()
        //    }
        //}

        onTextEdited: {
            if (newTextInput.text === "")
                return

            newTextInput.visible = false

            root.insert(newTextInput.index, newTextInput.text, ConditionListModel.Intermediate)

            newTextInput.clear()

            // Set focus on the newly created item
            let newItem = repeater.itemAt(newTextInput.index)
            newItem.forceActiveFocus()
        }

        Keys.onPressed: function (event) {
            if (event.key === Qt.Key_Backspace) {
                if (root.textInputActive()) {
                    let previousIndex = newTextInput.index - 1
                    if (previousIndex < 0)
                        return

                    let item = repeater.itemAt(previousIndex)
                    item.setCursorEnd()
                    item.forceActiveFocus()
                    popup.close()
                }
            }
        }
    }

    SuggestionPopup {
        id: popup

        style: StudioTheme.Values.connectionPopupControlStyle

        x: 0
        y: root.height
        width: root.width

        //onOpened: console.log("POPUP opened")
        //onClosed: console.log("POPUP closed")

        onSelect: function(value) {
            newTextInput.visible = true
            newTextInput.forceActiveFocus()

            if (root.shadowPillVisible) { // Active shadow pill
                root.remove(root.shadowPillIndex)
                root.shadowPillIndex = -1
            }

            root.insert(newTextInput.index, value, ConditionListModel.Variable)

            // Clear search, reset stack view and tree model
            popup.reset()
        }

        onSearchActiveChanged: {
            if (popup.searchActive) {
                root.heightBeforeShadowPill = flow.childrenRect.height
                root.insert(newTextInput.index, "...", ConditionListModel.Shadow)
                root.shadowPillIndex = newTextInput.index
            } else {
                if (!root.shadowPillVisible)
                    return

                root.remove(root.shadowPillIndex)
                root.shadowPillIndex = -1
            }
        }

        onEntered: function(value) {
            if (!popup.searchActive) {
                if (!root.shadowPillVisible) {
                    root.heightBeforeShadowPill = flow.childrenRect.height
                    root.shadowPillIndex = newTextInput.index
                    root.insert(newTextInput.index, value, ConditionListModel.Shadow)
                } else {
                    root.setValue(root.shadowPillIndex, value)
                }
            } else {
                root.setValue(root.shadowPillIndex, value)
            }
        }

        onExited: function(value) {
            let shadowItem = repeater.itemAt(root.shadowPillIndex)

            if (!popup.searchActive) {
                if (root.shadowPillVisible && shadowItem?.value === value) {
                    root.remove(root.shadowPillIndex)
                    root.shadowPillIndex = -1
                }
            } else {
                // Reset to 3 dots if still the same value as the exited item
                if (shadowItem?.value === value)
                    root.setValue(root.shadowPillIndex, "...")
            }
        }
    }

    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Left) {
            if (root.textInputActive()) {
                let previousIndex = newTextInput.index - 1
                if (previousIndex < 0)
                    return

                let item = repeater.itemAt(previousIndex)
                item.setCursorEnd()
                item.forceActiveFocus()
                popup.close()
            } else {
                if (flow.focusIndex < 0)
                    return

                root.placeCursor(flow.focusIndex)
            }
        } else if (event.key === Qt.Key_Right) {
            if (root.textInputActive()) {
                let nextIndex = newTextInput.index
                if (nextIndex >= repeater.count)
                    return

                let item = repeater.itemAt(nextIndex)
                item.setCursorBegin()
                item.forceActiveFocus()
                popup.close()
            } else {
                root.placeCursor(flow.focusIndex + 1)
            }
        }
    }
}
