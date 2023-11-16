// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Item {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    enum Interaction { None, TextEdit, Key }

    property int currentInteraction: FilterComboBox.Interaction.None

    property alias model: sortFilterModel.model
    property alias items: sortFilterModel.items
    property alias delegate: sortFilterModel.delegate

    property alias font: textInput.font

    // This indicates if the value was committed or the user is still editing
    property bool editing: false

    // This is the actual filter that is applied on the model
    property string filter: ""
    property bool filterActive: control.filter !== ""

    // Accept arbitrary input or only items from the model
    property bool allowUserInput: false

    property alias editText: textInput.text
    property int highlightedIndex: -1 // items index
    property int currentIndex: -1 // items index

    property string autocompleteString: ""

    property bool __isCompleted: false

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: (actionIndicator.hover || textInput.hover || checkIndicator.hover)
                         && control.enabled
    property alias edit: textInput.edit
    property alias open: popup.visible

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: control.style.actionIndicatorSize.width
    property real __actionIndicatorHeight: control.style.actionIndicatorSize.height

    property bool dirty: false // user modification flag

    property bool escapePressed: false

    signal accepted()
    signal activated(int index)
    signal compressedActivated(int index, int reason)

    enum ActivatedReason { EditingFinished, Other }

    property alias popup: popup
    property alias popupScrollBar: popupScrollBar
    property alias popupMouseArea: popupMouseArea

    property bool hasActiveDrag: false // an item that can be dropped here is being dragged
    property bool hasActiveHoverDrag: false // an item that can be dropped her is being hovered on top

    width: control.style.controlSize.width
    height: control.style.controlSize.height
    implicitHeight: control.style.controlSize.width

    function selectItem(itemsIndex) {
        textInput.text = sortFilterModel.items.get(itemsIndex).model.name
        control.currentIndex = itemsIndex
        control.finishEditing()
        control.activated(itemsIndex)
    }

    function submitValue() {
        if (!control.allowUserInput) {
            // If input isn't according to any item in the model, don't finish editing
            if (control.highlightedIndex === -1)
                return

            control.selectItem(control.highlightedIndex)
        } else {
            control.currentIndex = -1

            // Only trigger the signal, if the value was modified
            if (control.dirty) {
                myTimer.stop()
                control.dirty = false
                control.editText = control.editText.trim()
            }

            control.finishEditing()
            control.accepted()
        }
    }

    function finishEditing() {
        control.editing = false
        control.filter = ""
        control.autocompleteString = ""
        textInput.focus = false // Remove focus from text field
        popup.close()
    }

    function increaseVisibleIndex() {
        let numItems = sortFilterModel.visibleGroup.count
        if (!numItems)
            return

        if (control.highlightedIndex === -1) // Nothing is selected
            control.setHighlightedIndexVisible(0)
        else {
            let currentVisibleIndex = sortFilterModel.items.get(control.highlightedIndex).visibleIndex
            ++currentVisibleIndex

            if (currentVisibleIndex > numItems - 1)
                currentVisibleIndex = 0

            control.setHighlightedIndexVisible(currentVisibleIndex)
        }
    }

    function decreaseVisibleIndex() {
        let numItems = sortFilterModel.visibleGroup.count
        if (!numItems)
            return

        if (control.highlightedIndex === -1) // Nothing is selected
            control.setHighlightedIndexVisible(numItems - 1)
        else {
            let currentVisibleIndex = sortFilterModel.items.get(control.highlightedIndex).visibleIndex
            --currentVisibleIndex

            if (currentVisibleIndex < 0)
                currentVisibleIndex = numItems - 1

            control.setHighlightedIndexVisible(currentVisibleIndex)
        }
    }

    function updateHighlightedIndex() {
        // Check if current index is still part of the filtered list, if not set it to 0
        if (control.highlightedIndex !== -1 && !sortFilterModel.items.get(control.highlightedIndex).inVisible) {
            control.setHighlightedIndexVisible(0)
        } else {
            // Needs to be set in order for ListView to keep its currenIndex up to date, so
            // scroll position gets updated according to the higlighted item
            control.setHighlightedIndexItems(control.highlightedIndex)
        }
    }

    function setHighlightedIndexItems(itemsIndex) { // items group index
        control.highlightedIndex = itemsIndex

        if (itemsIndex === -1)
            listView.currentIndex = -1
        else
            listView.currentIndex = sortFilterModel.items.get(itemsIndex).visibleIndex
    }

    function setHighlightedIndexVisible(visibleIndex) { // visible group index
        if (visibleIndex === -1)
            control.highlightedIndex = -1
        else
            control.highlightedIndex = sortFilterModel.visibleGroup.get(visibleIndex).itemsIndex

        listView.currentIndex = visibleIndex
    }

    function updateAutocomplete() {
        if (control.highlightedIndex === -1)
            control.autocompleteString = ""
        else {
            let suggestion = sortFilterModel.items.get(control.highlightedIndex).model.name
            control.autocompleteString = suggestion.substring(textInput.text.length)
        }
    }

    // TODO is this already case insensitiv?!
    function find(text) {
        for (let i = 0; i < sortFilterModel.items.count; ++i)
            if (sortFilterModel.items.get(i).model.name === text)
                return i

        return -1
    }

    Timer {
        id: myTimer
        property int activatedIndex
        repeat: false
        running: false
        interval: 100
        onTriggered: control.compressedActivated(myTimer.activatedIndex,
                                                 ComboBox.ActivatedReason.Other)
    }

    onActivated: function(index) {
        myTimer.activatedIndex = index
        myTimer.restart()
    }

    onHighlightedIndexChanged: {
        if (control.editing || (control.editText === "" && control.allowUserInput))
            control.updateAutocomplete()
    }

    DelegateModel {
        id: noMatchesModel

        model: ListModel {
            ListElement { name: "No matches" }
        }

        delegate: ItemDelegate {
            id: noMatchesDelegate
            width: popup.width - popup.leftPadding - popup.rightPadding
                   - (popupScrollBar.visible ? popupScrollBar.contentItem.implicitWidth + 2
                                             : 0) // TODO Magic number
            height: control.style.controlSize.height - 2 * control.style.borderWidth
            padding: 0

            contentItem: Text {
                leftPadding: control.style.inputHorizontalPadding
                text: name
                font.italic: true
                color: control.style.text.idle
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                x: 0
                y: 0
                width: noMatchesDelegate.width
                height: noMatchesDelegate.height
                color: "transparent"
            }
        }
    }

    SortFilterModel {
        id: sortFilterModel

        filterAcceptsItem: function(item) {
            return item.name.toLowerCase().startsWith(control.filter.toLowerCase())
        }

        lessThan: function(left, right) {
            if (left.group === right.group) {
              return left.name.toLowerCase().localeCompare(right.name.toLowerCase())
            }

            return left.group - right.group
        }

        delegate: ItemDelegate {
            id: delegateRoot
            width: popup.width - popup.leftPadding - popup.rightPadding
                   - (popupScrollBar.visible ? popupScrollBar.contentItem.implicitWidth + 2
                                             : 0) // TODO Magic number
            height: control.style.controlSize.height - 2 * control.style.borderWidth
            padding: 0
            hoverEnabled: true
            highlighted: control.highlightedIndex === delegateRoot.DelegateModel.itemsIndex

            onHoveredChanged: {
                if (delegateRoot.hovered && !popupMouseArea.active)
                    control.setHighlightedIndexItems(delegateRoot.DelegateModel.itemsIndex)
            }

            onClicked: control.selectItem(delegateRoot.DelegateModel.itemsIndex)

            contentItem: Text {
                leftPadding: 8
                text: name
                color: control.currentIndex === delegateRoot.DelegateModel.itemsIndex
                       ? control.style.text.selectedText
                       : control.style.text.idle
                font: textInput.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                x: 0
                y: 0
                width: delegateRoot.width
                height: delegateRoot.height
                color: {
                    if (!itemDelegate.enabled)
                        return "transparent"

                    if (itemDelegate.hovered
                            && control.currentIndex === delegateRoot.DelegateModel.itemsIndex)
                        return control.style.interactionHover

                    if (control.currentIndex === delegateRoot.DelegateModel.itemsIndexx)
                        return control.style.interaction

                    if (itemDelegate.hovered)
                        return control.style.background.hover

                    return "transparent"
                }
            }
        }

        onUpdated: {
            if (!control.__isCompleted)
                return

            if (sortFilterModel.count === 0)
                control.setHighlightedIndexVisible(-1)
            else {
                if (control.highlightedIndex === -1 && !control.allowUserInput)
                    control.setHighlightedIndexVisible(0)
            }
        }
    }

    Row {
        ActionIndicator {
            id: actionIndicator
            style: control.style
            __parentControl: control
            x: 0
            y: 0
            width: actionIndicator.visible ? control.__actionIndicatorWidth : 0
            height: actionIndicator.visible ? control.__actionIndicatorHeight : 0
        }

        TextInput {
            id: textInput

            property bool hover: textInputMouseArea.containsMouse && textInput.enabled
            property bool edit: textInput.activeFocus
            property string preFocusText: ""

            x: 0
            y: 0
            z: 2
            width: control.width - actionIndicator.width
            height: control.height
            leftPadding: control.style.inputHorizontalPadding
            rightPadding: control.style.inputHorizontalPadding + checkIndicator.width
                          + control.style.borderWidth
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            color: control.style.text.idle
            selectionColor: control.style.text.selection
            selectedTextColor: control.style.text.selectedText
            selectByMouse: true
            clip: true

            Rectangle {
                id: textInputBackground
                z: -1
                width: textInput.width
                height: textInput.height
                color: control.style.background.idle
                border.color: control.style.border.idle
                border.width: control.style.borderWidth
            }

            MouseArea {
                id: textInputMouseArea
                anchors.fill: parent
                enabled: true
                hoverEnabled: true
                propagateComposedEvents: true
                acceptedButtons: Qt.LeftButton
                cursorShape: Qt.PointingHandCursor
                onPressed: function(mouse) {
                    textInput.forceActiveFocus()
                    mouse.accepted = false
                }

                // Stop scrollable views from scrolling while ComboBox is in edit mode and the mouse
                // pointer is on top of it. We might add wheel selection in the future.
                onWheel: function(wheel) {
                    wheel.accepted = control.edit
                }
            }

            onEditingFinished: {
                if (control.escapePressed) {
                    control.escapePressed = false
                    control.editText = textInput.preFocusText
                } else {
                    if (control.currentInteraction === FilterComboBox.Interaction.TextEdit) {
                        if (control.dirty)
                            control.submitValue()
                    } else if (control.currentInteraction === FilterComboBox.Interaction.Key) {
                        control.selectItem(control.highlightedIndex)
                    }
                }

                sortFilterModel.update()
            }

            onTextEdited: {
                control.currentInteraction = FilterComboBox.Interaction.TextEdit
                control.editing = true
                popupMouseArea.active = true
                control.dirty = true

                if (textInput.text !== "")
                    control.filter = textInput.text
                else {
                    control.filter = ""
                    control.autocompleteString = ""
                }

                if (!popup.visible)
                    popup.open()

                sortFilterModel.update()

                if (!control.allowUserInput)
                    control.updateHighlightedIndex()
                else
                    control.setHighlightedIndexVisible(-1)

                control.updateAutocomplete()
            }

            onActiveFocusChanged: {
                if (textInput.activeFocus) {
                    popup.open()
                    textInput.preFocusText = textInput.text
                } else
                    popup.close()
            }

            states: [
                State {
                    name: "default"
                    when: control.enabled && !textInput.edit && !control.hover && !control.open
                          && !control.hasActiveDrag
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.idle
                        border.color: control.style.border.idle
                    }
                    PropertyChanges {
                        target: textInputMouseArea
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton
                    }
                },
                State {
                    name: "acceptsDrag"
                    when: control.enabled && control.hasActiveDrag && !control.hasActiveHoverDrag
                    PropertyChanges {
                        target: textInputBackground
                        border.color: control.style.interaction
                    }
                },
                State {
                    name: "dragHover"
                    when: control.enabled && control.hasActiveHoverDrag
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.interaction
                        border.color: control.style.interaction
                    }
                },
                State {
                    name: "globalHover"
                    when: control.hover && !textInput.hover && !textInput.edit && !control.open
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.globalHover
                    }
                },
                State {
                    name: "hover"
                    when: textInput.hover && control.hover && !textInput.edit
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.hover
                    }
                },
                State {
                    name: "edit"
                    when: control.edit
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.interaction
                        border.color: control.style.border.interaction
                    }
                    PropertyChanges {
                        target: textInputMouseArea
                        cursorShape: Qt.IBeamCursor
                        acceptedButtons: Qt.NoButton
                    }
                },
                State {
                    name: "disable"
                    when: !control.enabled
                    PropertyChanges {
                        target: textInputBackground
                        color: control.style.background.disabled
                        border.color: control.style.border.disabled
                    }
                    PropertyChanges {
                        target: textInput
                        color: control.style.text.disabled
                    }
                }
            ]

            Text {
                id: tmpSelectionName
                visible: control.autocompleteString !== "" && control.open
                text: control.autocompleteString
                x: textInput.leftPadding + textMetrics.advanceWidth
                y: (textInput.height - Math.ceil(tmpSelectionTextMetrics.height)) / 2
                color: "gray" // TODO proper color value
                font: textInput.font
                renderType: textInput.renderType

                TextMetrics {
                    id: textMetrics
                    font: textInput.font
                    text: textInput.text
                }
                TextMetrics {
                    id: tmpSelectionTextMetrics
                    font: tmpSelectionName.font
                    text: "Xq"
                }
            }

            Rectangle {
                id: checkIndicator

                property bool hover: checkIndicatorMouseArea.containsMouse && checkIndicator.enabled
                property bool pressed: checkIndicatorMouseArea.containsPress
                property bool checked: popup.visible

                x: textInput.width - checkIndicator.width - control.style.borderWidth
                y: control.style.borderWidth
                width: control.style.squareControlSize.width - control.style.borderWidth
                height: textInput.height  - (control.style.borderWidth * 2)
                color: control.style.background.idle
                border.width: 0

                MouseArea {
                    id: checkIndicatorMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (popup.visible)
                            popup.close()
                        else
                            popup.open()

                        if (!textInput.activeFocus) {
                            textInput.forceActiveFocus()
                            textInput.selectAll()
                        }
                    }
                }

                T.Label {
                    id: checkIndicatorIcon
                    anchors.fill: parent
                    color: control.style.icon.idle
                    text: StudioTheme.Constants.upDownSquare2
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: control.style.baseIconFontSize
                    font.family: StudioTheme.Constants.iconFont.family
                }

                states: [
                    State {
                        name: "default"
                        when: control.enabled && checkIndicator.enabled && !control.edit
                              && !checkIndicator.hover && !control.hover
                              && !checkIndicator.checked && !control.hasActiveHoverDrag
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.background.idle
                        }
                    },
                    State {
                        name: "dragHover"
                        when: control.enabled && control.hasActiveHoverDrag
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.background.interaction
                        }
                    },
                    State {
                        name: "globalHover"
                        when: control.enabled && checkIndicator.enabled
                              && !checkIndicator.hover && control.hover && !control.edit
                              && !checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.background.globalHover
                        }
                    },
                    State {
                        name: "hover"
                        when: control.enabled && checkIndicator.enabled
                              && checkIndicator.hover && control.hover && !checkIndicator.pressed
                              && !checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.background.hover
                        }
                    },
                    State {
                        name: "check"
                        when: checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: control.style.icon.idle
                        }
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.interaction
                        }
                    },
                    State {
                        name: "press"
                        when: control.enabled && checkIndicator.enabled
                              && checkIndicator.pressed
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: control.style.icon.idle
                        }
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.interaction
                        }
                    },
                    State {
                        name: "disable"
                        when: !control.enabled
                        PropertyChanges {
                            target: checkIndicator
                            color: control.style.background.disabled
                        }
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: control.style.icon.disabled
                        }
                    }
                ]
            }
        }
    }

    T.Popup {
        id: popup
        x: textInput.x
        y: textInput.height
        width: textInput.width
        height: Math.min(popup.contentItem.implicitHeight + popup.topPadding + popup.bottomPadding,
                         control.Window.height - popup.topMargin - popup.bottomMargin,
                         control.style.maxComboBoxPopupHeight)
        padding: control.style.borderWidth
        margins: 0 // If not defined margin will be -1
        closePolicy: T.Popup.NoAutoClose

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: listView.contentHeight
            highlightMoveVelocity: -1
            boundsBehavior: Flickable.StopAtBounds
            flickDeceleration: 10000

            model: {
                if (popup.visible)
                    return sortFilterModel.count ? sortFilterModel : noMatchesModel

                return null
            }

            HoverHandler { id: hoverHandler }

            ScrollBar.vertical: TransientScrollBar {
                id: popupScrollBar
                parent: listView
                x: listView.width - popupScrollBar.width
                y: 0
                height: listView.availableHeight
                orientation: Qt.Vertical

                show: (hoverHandler.hovered || popupScrollBar.inUse)
                      && popupScrollBar.isNeeded
            }
        }

        background: Rectangle {
            color: control.style.popup.background
            border.width: 0
        }

        onOpened: {
            // Reset the highlightedIndex of ListView as binding with condition didn't work
            if (control.highlightedIndex !== -1)
                control.setHighlightedIndexItems(control.highlightedIndex)
        }

        onAboutToShow: {
            // Select first item in list
            if (control.highlightedIndex === -1 && sortFilterModel.count && !control.allowUserInput)
                control.setHighlightedIndexVisible(0)
        }

        MouseArea {
            // This is MouseArea is intended to block the hovered property of an ItemDelegate
            // when the ListView changes due to Key interaction.

            id: popupMouseArea
            property bool active: true

            anchors.fill: parent
            enabled: popup.visible && popupMouseArea.active
            hoverEnabled: popupMouseArea.enabled
            onPositionChanged: { popupMouseArea.active = false }
        }
    }

    Keys.onDownPressed: {
        if (!sortFilterModel.visibleGroup.count)
            return

        control.currentInteraction = FilterComboBox.Interaction.Key
        control.increaseVisibleIndex()

        popupMouseArea.active = true
    }

    Keys.onUpPressed: {
        if (!sortFilterModel.visibleGroup.count)
            return

        control.currentInteraction = FilterComboBox.Interaction.Key
        control.decreaseVisibleIndex()

        popupMouseArea.active = true
    }

    Keys.onEscapePressed: {
        control.escapePressed = true
        control.finishEditing()
    }

    Component.onCompleted: {
        let index = control.find(control.editText)
        control.currentIndex = index
        control.highlightedIndex = index // TODO might not be intended
        control.__isCompleted = true
    }
}
