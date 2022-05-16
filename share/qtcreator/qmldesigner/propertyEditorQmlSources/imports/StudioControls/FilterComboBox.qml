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
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

Item {
    id: root

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
    property bool filterActive: root.filter !== ""

    // Accept arbitrary input or only items from the model
    property bool allowUserInput: false

    property alias editText: textInput.text
    property int highlightedIndex: -1 // items index
    property int currentIndex: -1 // items index

    property string autocompleteString: ""

    property bool __isCompleted: false

    property alias actionIndicator: actionIndicator

    // This property is used to indicate the global hover state
    property bool hover: actionIndicator.hover || textInput.hover || checkIndicator.hover
    property alias edit: textInput.edit
    property alias open: popup.visible

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    property bool dirty: false // user modification flag

    property bool escapePressed: false

    signal accepted()
    signal activated(int index)
    signal compressedActivated(int index, int reason)

    enum ActivatedReason { EditingFinished, Other }

    property alias popup: popup
    property alias popupScrollBar: popupScrollBar
    property alias popupMouseArea: popupMouseArea

    width: StudioTheme.Values.defaultControlWidth
    height: StudioTheme.Values.defaultControlHeight
    implicitHeight: StudioTheme.Values.defaultControlHeight

    function selectItem(itemsIndex) {
        textInput.text = sortFilterModel.items.get(itemsIndex).model.name
        root.currentIndex = itemsIndex
        root.finishEditing()
        root.activated(itemsIndex)
    }

    function submitValue() {
        if (!root.allowUserInput) {
            // If input isn't according to any item in the model, don't finish editing
            if (root.highlightedIndex === -1)
                return

            root.selectItem(root.highlightedIndex)
        } else {
            root.currentIndex = -1

            // Only trigger the signal, if the value was modified
            if (root.dirty) {
                myTimer.stop()
                root.dirty = false
                root.editText = root.editText.trim()
            }

            root.finishEditing()
            root.accepted()
        }
    }

    function finishEditing() {
        root.editing = false
        root.filter = ""
        root.autocompleteString = ""
        textInput.focus = false // Remove focus from text field
        popup.close()
    }

    function increaseVisibleIndex() {
        let numItems = sortFilterModel.visibleGroup.count
        if (!numItems)
            return

        if (root.highlightedIndex === -1) // Nothing is selected
            root.setHighlightedIndexVisible(0)
        else {
            let currentVisibleIndex = sortFilterModel.items.get(root.highlightedIndex).visibleIndex
            ++currentVisibleIndex

            if (currentVisibleIndex > numItems - 1)
                currentVisibleIndex = 0

            root.setHighlightedIndexVisible(currentVisibleIndex)
        }
    }

    function decreaseVisibleIndex() {
        let numItems = sortFilterModel.visibleGroup.count
        if (!numItems)
            return

        if (root.highlightedIndex === -1) // Nothing is selected
            root.setHighlightedIndexVisible(numItems - 1)
        else {
            let currentVisibleIndex = sortFilterModel.items.get(root.highlightedIndex).visibleIndex
            --currentVisibleIndex

            if (currentVisibleIndex < 0)
                currentVisibleIndex = numItems - 1

            root.setHighlightedIndexVisible(currentVisibleIndex)
        }
    }

    function updateHighlightedIndex() {
        // Check if current index is still part of the filtered list, if not set it to 0
        if (root.highlightedIndex !== -1 && !sortFilterModel.items.get(root.highlightedIndex).inVisible) {
            root.setHighlightedIndexVisible(0)
        } else {
            // Needs to be set in order for ListView to keep its currenIndex up to date, so
            // scroll position gets updated according to the higlighted item
            root.setHighlightedIndexItems(root.highlightedIndex)
        }
    }

    function setHighlightedIndexItems(itemsIndex) { // items group index
        root.highlightedIndex = itemsIndex

        if (itemsIndex === -1)
            listView.currentIndex = -1
        else
            listView.currentIndex = sortFilterModel.items.get(itemsIndex).visibleIndex
    }

    function setHighlightedIndexVisible(visibleIndex) { // visible group index
        if (visibleIndex === -1)
            root.highlightedIndex = -1
        else
            root.highlightedIndex = sortFilterModel.visibleGroup.get(visibleIndex).itemsIndex

        listView.currentIndex = visibleIndex
    }

    function updateAutocomplete() {
        if (root.highlightedIndex === -1)
            root.autocompleteString = ""
        else {
            let suggestion = sortFilterModel.items.get(root.highlightedIndex).model.name
            root.autocompleteString = suggestion.substring(textInput.text.length)
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
        onTriggered: root.compressedActivated(myTimer.activatedIndex,
                                              ComboBox.ActivatedReason.Other)
    }

    onActivated: function(index) {
        myTimer.activatedIndex = index
        myTimer.restart()
    }

    onHighlightedIndexChanged: {
        if (root.editing || (root.editText === "" && root.allowUserInput))
            root.updateAutocomplete()
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
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            padding: 0

            contentItem: Text {
                leftPadding: StudioTheme.Values.inputHorizontalPadding
                text: name
                font.italic: true
                color: StudioTheme.Values.themeTextColor
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
            return item.name.toLowerCase().startsWith(root.filter.toLowerCase())
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
            height: StudioTheme.Values.height - 2 * StudioTheme.Values.border
            padding: 0
            hoverEnabled: true
            highlighted: root.highlightedIndex === delegateRoot.DelegateModel.itemsIndex

            onHoveredChanged: {
                if (delegateRoot.hovered && !popupMouseArea.active)
                    root.setHighlightedIndexItems(delegateRoot.DelegateModel.itemsIndex)
            }

            onClicked: root.selectItem(delegateRoot.DelegateModel.itemsIndex)

            indicator: Item {
                id: itemDelegateIconArea
                width: delegateRoot.height
                height: delegateRoot.height

                T.Label {
                    id: itemDelegateIcon
                    text: StudioTheme.Constants.tickIcon
                    color: delegateRoot.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                    : StudioTheme.Values.themeTextColor
                    font.family: StudioTheme.Constants.iconFont.family
                    font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
                    visible: root.currentIndex === delegateRoot.DelegateModel.itemsIndex ? true
                                                                                         : false
                    anchors.fill: parent
                    renderType: Text.NativeRendering
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            contentItem: Text {
                leftPadding: itemDelegateIconArea.width
                text: name
                color: delegateRoot.highlighted ? StudioTheme.Values.themeTextSelectedTextColor
                                                : StudioTheme.Values.themeTextColor
                font: textInput.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                x: 0
                y: 0
                width: delegateRoot.width
                height: delegateRoot.height
                color: delegateRoot.highlighted ? StudioTheme.Values.themeInteraction
                                                : "transparent"
            }
        }

        onUpdated: {
            if (!root.__isCompleted)
                return

            if (sortFilterModel.count === 0)
                root.setHighlightedIndexVisible(-1)
            else {
                if (root.highlightedIndex === -1 && !root.allowUserInput)
                    root.setHighlightedIndexVisible(0)
            }
        }
    }

    Row {
        ActionIndicator {
            id: actionIndicator
            myControl: root
            x: 0
            y: 0
            width: actionIndicator.visible ? root.__actionIndicatorWidth : 0
            height: actionIndicator.visible ? root.__actionIndicatorHeight : 0
        }

        TextInput {
            id: textInput

            property bool hover: textInputMouseArea.containsMouse && textInput.enabled
            property bool edit: textInput.activeFocus
            property string preFocusText: ""

            x: 0
            y: 0
            z: 2
            width: root.width - actionIndicator.width
            height: root.height
            leftPadding: StudioTheme.Values.inputHorizontalPadding
            rightPadding: StudioTheme.Values.inputHorizontalPadding + checkIndicator.width
                          + StudioTheme.Values.border
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            color: StudioTheme.Values.themeTextColor
            selectionColor: StudioTheme.Values.themeTextSelectionColor
            selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor
            selectByMouse: true
            clip: true

            Rectangle {
                id: textInputBackground
                z: -1
                width: textInput.width
                height: textInput.height
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
                border.width: StudioTheme.Values.border
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
                    wheel.accepted = root.edit
                }
            }

            onEditingFinished: {
                if (root.escapePressed) {
                    root.escapePressed = false
                    root.editText = textInput.preFocusText
                } else {
                    if (root.currentInteraction === FilterComboBox.Interaction.TextEdit) {
                        if (root.dirty)
                            root.submitValue()
                    } else if (root.currentInteraction === FilterComboBox.Interaction.Key) {
                        root.selectItem(root.highlightedIndex)
                    }
                }

                sortFilterModel.update()
            }

            onTextEdited: {
                root.currentInteraction = FilterComboBox.Interaction.TextEdit
                root.editing = true
                popupMouseArea.active = true
                root.dirty = true

                if (textInput.text !== "")
                    root.filter = textInput.text
                else {
                    root.filter = ""
                    root.autocompleteString = ""
                }

                if (!popup.visible)
                    popup.open()

                sortFilterModel.update()

                if (!root.allowUserInput)
                    root.updateHighlightedIndex()
                else
                    root.setHighlightedIndexVisible(-1)

                root.updateAutocomplete()
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
                    when: root.enabled && !textInput.edit && !root.hover && !root.open
                    PropertyChanges {
                        target: textInputBackground
                        color: StudioTheme.Values.themeControlBackground
                    }
                    PropertyChanges {
                        target: textInputMouseArea
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton
                    }
                },
                State {
                    name: "globalHover"
                    when: root.hover && !textInput.hover && !textInput.edit && !root.open
                    PropertyChanges {
                        target: textInputBackground
                        color: StudioTheme.Values.themeControlBackgroundGlobalHover
                    }
                },
                State {
                    name: "hover"
                    when: textInput.hover && root.hover && !textInput.edit
                    PropertyChanges {
                        target: textInputBackground
                        color: StudioTheme.Values.themeControlBackgroundHover
                    }
                },
                State {
                    name: "edit"
                    when: root.edit
                    PropertyChanges {
                        target: textInputBackground
                        color: StudioTheme.Values.themeControlBackgroundInteraction
                        border.color: StudioTheme.Values.themeControlOutlineInteraction
                    }
                    PropertyChanges {
                        target: textInputMouseArea
                        cursorShape: Qt.IBeamCursor
                        acceptedButtons: Qt.NoButton
                    }
                },
                State {
                    name: "disable"
                    when: !root.enabled
                    PropertyChanges {
                        target: textInputBackground
                        color: StudioTheme.Values.themeControlBackgroundDisabled
                    }
                    PropertyChanges {
                        target: textInput
                        color: StudioTheme.Values.themeTextColorDisabled
                    }
                }
            ]

            Text {
                visible: root.autocompleteString !== ""
                text: root.autocompleteString
                x: textInput.leftPadding + textMetrics.advanceWidth
                y: (textInput.height - Math.ceil(textMetrics.height)) / 2
                color: "gray" // TODO proper color value
                font: textInput.font
                renderType: textInput.renderType
            }

            TextMetrics {
                id: textMetrics
                font: textInput.font
                text: textInput.text
            }

            Rectangle {
                id: checkIndicator

                property bool hover: checkIndicatorMouseArea.containsMouse && checkIndicator.enabled
                property bool pressed: checkIndicatorMouseArea.containsPress
                property bool checked: popup.visible

                x: textInput.width - checkIndicator.width - StudioTheme.Values.border
                y: StudioTheme.Values.border
                width: StudioTheme.Values.height - StudioTheme.Values.border
                height: textInput.height  - (StudioTheme.Values.border * 2)
                color: StudioTheme.Values.themeControlBackground
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
                    color: StudioTheme.Values.themeTextColor
                    text: StudioTheme.Constants.upDownSquare2
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: StudioTheme.Values.sliderControlSizeMulti
                    font.family: StudioTheme.Constants.iconFont.family
                }

                states: [
                    State {
                        name: "default"
                        when: root.enabled && checkIndicator.enabled && !root.edit
                              && !checkIndicator.hover && !root.hover
                              && !checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeControlBackground
                        }
                    },
                    State {
                        name: "globalHover"
                        when: root.enabled && checkIndicator.enabled
                              && !checkIndicator.hover && root.hover && !root.edit
                              && !checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeControlBackgroundGlobalHover
                        }
                    },
                    State {
                        name: "hover"
                        when: root.enabled && checkIndicator.enabled
                              && checkIndicator.hover && root.hover && !checkIndicator.pressed
                              && !checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeControlBackgroundHover
                        }
                    },
                    State {
                        name: "check"
                        when: checkIndicator.checked
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: StudioTheme.Values.themeIconColor
                        }
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeInteraction
                        }
                    },
                    State {
                        name: "press"
                        when: root.enabled && checkIndicator.enabled
                              && checkIndicator.pressed
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: StudioTheme.Values.themeIconColor
                        }
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeInteraction
                        }
                    },
                    State {
                        name: "disable"
                        when: !root.enabled
                        PropertyChanges {
                            target: checkIndicator
                            color: StudioTheme.Values.themeControlBackgroundDisabled
                        }
                        PropertyChanges {
                            target: checkIndicatorIcon
                            color: StudioTheme.Values.themeTextColorDisabled
                        }
                    }
                ]
            }
        }
    }

    T.Popup {
        id: popup
        x: textInput.x + StudioTheme.Values.border
        y: textInput.height
        width: textInput.width - (StudioTheme.Values.border * 2)
        height: Math.min(popup.contentItem.implicitHeight + popup.topPadding + popup.bottomPadding,
                         root.Window.height - popup.topMargin - popup.bottomMargin,
                         StudioTheme.Values.maxComboBoxPopupHeight)
        padding: StudioTheme.Values.border
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

            ScrollBar.vertical: ScrollBar {
                id: popupScrollBar
                visible: listView.height < listView.contentHeight
            }
        }

        background: Rectangle {
            color: StudioTheme.Values.themePopupBackground
            border.width: 0
        }

        onOpened: {
            // Reset the highlightedIndex of ListView as binding with condition didn't work
            if (root.highlightedIndex !== -1)
                root.setHighlightedIndexItems(root.highlightedIndex)
        }

        onAboutToShow: {
            // Select first item in list
            if (root.highlightedIndex === -1 && sortFilterModel.count && !root.allowUserInput)
                root.setHighlightedIndexVisible(0)
        }

        MouseArea {
            // This is MouseArea is intended to block the hovered property of an ItemDelegate
            // when the ListView changes due to Key interaction.

            id: popupMouseArea
            property bool active: true

            anchors.fill: parent
            enabled: popup.visible && popupMouseArea.active
            hoverEnabled: true
            onPositionChanged: { popupMouseArea.active = false }
        }
    }

    Keys.onDownPressed: {
        if (!sortFilterModel.visibleGroup.count)
            return

        root.currentInteraction = FilterComboBox.Interaction.Key
        root.increaseVisibleIndex()

        popupMouseArea.active = true
    }

    Keys.onUpPressed: {
        if (!sortFilterModel.visibleGroup.count)
            return

        root.currentInteraction = FilterComboBox.Interaction.Key
        root.decreaseVisibleIndex()

        popupMouseArea.active = true
    }

    Keys.onEscapePressed: {
        root.escapePressed = true
        root.finishEditing()
    }

    Component.onCompleted: {
        let index = root.find(root.editText)
        root.currentIndex = index
        root.highlightedIndex = index // TODO might not be intended

        root.__isCompleted = true
    }
}
