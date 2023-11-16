/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme
import StatesEditorBackend

Item {
    id: root
    clip: true

    anchors {
        horizontalCenter: parent.horizontalCenter
        verticalCenter: parent.verticalCenter
    }

    property alias thumbnailImageSource: thumbnailImage.source
    property alias stateName: stateNameField.text
    property alias whenCondition: whenCondition.text

    property alias defaultChecked: defaultButton.checked
    property alias menuChecked: menuButton.checked
    property bool baseState: false
    property bool isTiny: false
    property bool propertyChangesVisible: propertyChangesModel.propertyChangesVisible
    property bool isChecked: false

    property bool hasExtend: false
    property string extendString: ""
    property bool extendedState: false

    property bool hasWhenCondition: false

    property bool menuOpen: stateMenu.opened
    property bool blockDragHandler: false

    property Item dragParent

    property int visualIndex: 0

    property int internalNodeId

    signal focusSignal
    signal defaultClicked
    signal clone
    signal extend
    signal remove
    signal stateNameFinished

    signal grabbing
    signal letGo

    property alias dragActive: dragHandler.active

    function checkAnnotation() {
        return StatesEditorBackend.statesEditorModel.hasAnnotation(root.internalNodeId)
    }

    function setPropertyChangesVisible(value) {
        root.propertyChangesVisible = value
        propertyChangesModel.setPropertyChangesVisible(value)
    }

    onIsTinyChanged: {
        if (root.isTiny) {
            buttonGrid.rows = 2
            buttonGrid.columns = 1
        } else {
            buttonGrid.columns = 2
            buttonGrid.rows = 1
        }
    }

    DragHandler {
        id: dragHandler
        enabled: !root.baseState && !root.extendedState && !root.blockDragHandler
        onGrabChanged: function (transition, point) {
            if (transition === PointerDevice.GrabPassive
                    || transition === PointerDevice.GrabExclusive)
                root.grabbing()

            if (transition === PointerDevice.UngrabPassive
                    || transition === PointerDevice.CancelGrabPassive
                    || transition === PointerDevice.UngrabExclusive
                    || transition === PointerDevice.CancelGrabExclusive)
                root.letGo()
        }
    }

    onDragActiveChanged: {
        if (root.dragActive)
            Drag.start()
        else
            Drag.drop()
    }

    Drag.active: dragHandler.active
    Drag.source: root
    Drag.hotSpot.x: root.width * 0.5
    Drag.hotSpot.y: root.height * 0.5

    Rectangle {
        id: stateBackground
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeInteraction
        border.width: root.isChecked ? 3 : 0
        radius: 10
        anchors.fill: parent

        readonly property int controlHeight: 25
        readonly property int thumbPadding: 10
        readonly property int thumbSpacing: 7

        property int innerWidth: root.width - 2 * stateBackground.thumbPadding
        property int innerHeight: root.height - 2 * stateBackground.thumbPadding - 2
                                  * stateBackground.thumbSpacing - 2 * stateBackground.controlHeight

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            enabled: true
            hoverEnabled: true
            propagateComposedEvents: true
            onClicked: root.focusSignal()
        }

        Column {
            padding: stateBackground.thumbPadding
            spacing: stateBackground.thumbSpacing

            Grid {
                id: buttonGrid
                columns: 2
                rows: 1
                spacing: stateBackground.thumbSpacing

                HelperWidgets.AbstractButton {
                    id: defaultButton
                    style: StudioTheme.Values.statesControlStyle
                    width: 50
                    height: stateBackground.controlHeight
                    checkedInverted: true
                    buttonIcon: qsTr("Default")
                    iconFont: StudioTheme.Constants.font
                    tooltip: qsTr("Set State as default")
                    onClicked: {
                        root.defaultClicked()
                        root.focusSignal()
                    }
                }

                StudioControls.TextField {
                    id: stateNameField
                    style: StudioTheme.Values.statesControlStyle

                    property string previousText

                    // This is the width for the "big" state
                    property int bigWidth: stateBackground.innerWidth - 2 * stateBackground.thumbSpacing
                                           - defaultButton.width - menuButton.width

                    width: root.isTiny ? stateBackground.innerWidth : stateNameField.bigWidth
                    height: stateBackground.controlHeight
                    actionIndicatorVisible: false
                    translationIndicatorVisible: false
                    placeholderText: qsTr("State Name")
                    visible: !root.baseState

                    onActiveFocusChanged: {
                        if (stateNameField.activeFocus)
                            root.focusSignal()
                    }

                    onEditingFinished: {
                        if (stateNameField.previousText === stateNameField.text)
                            return

                        stateNameField.previousText = stateNameField.text
                        root.stateNameFinished()
                    }

                    Component.onCompleted: stateNameField.previousText = stateNameField.text
                }

                Text {
                    id: baseStateLabel
                    width: root.isTiny ? stateBackground.innerWidth : stateNameField.bigWidth
                    height: stateBackground.controlHeight
                    visible: root.baseState
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("Base State")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 5
                    elide: Text.ElideRight
                }
            }

            Item {
                visible: !root.isTiny && !root.propertyChangesVisible
                width: stateBackground.innerWidth
                height: stateBackground.innerHeight

                Image {
                    anchors.fill: stateImageBackground
                    source: "images/checkers.png"
                    fillMode: Image.Tile
                }

                Rectangle {
                    id: stateImageBackground
                    x: Math.floor(
                           (stateBackground.innerWidth - thumbnailImage.paintedWidth) / 2) - StudioTheme.Values.border
                    y: Math.floor(
                           (stateBackground.innerHeight - thumbnailImage.paintedHeight) / 2) - StudioTheme.Values.border
                    width: Math.round(thumbnailImage.paintedWidth) + 2 * StudioTheme.Values.border
                    height: Math.round(thumbnailImage.paintedHeight) + 2 * StudioTheme.Values.border
                    color: "transparent"
                    border.width: StudioTheme.Values.border
                    border.color: StudioTheme.Values.themeStatePreviewOutline
                }

                Image {
                    id: thumbnailImage
                    anchors.centerIn: parent
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
            }

            PropertyChangesModel {
                id: propertyChangesModel
                modelNodeBackendProperty: StatesEditorBackend.statesEditorModel.stateModelNode(root.internalNodeId)
            }

            Text {
                visible: !root.isTiny && root.propertyChangesVisible && !propertyChangesModel.count
                width: stateBackground.innerWidth
                height: stateBackground.innerHeight
                text: qsTr("No Property Changes Available")
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.baseFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            ScrollView {
                id: scrollView
                visible: !root.isTiny && root.propertyChangesVisible && propertyChangesModel.count
                width: stateBackground.innerWidth
                height: stateBackground.innerHeight
                clip: true

                ScrollBar.horizontal: StateScrollBar {
                    parent: scrollView
                    x: scrollView.leftPadding
                    y: scrollView.height - height
                    width: scrollView.availableWidth
                    orientation: Qt.Horizontal
                    onPressedChanged: root.focusSignal()
                }

                ScrollBar.vertical: StateScrollBar {
                    parent: scrollView
                    x: scrollView.mirrored ? 0 : scrollView.width - width
                    y: scrollView.topPadding
                    height: scrollView.availableHeight
                    orientation: Qt.Vertical
                    onPressedChanged: root.focusSignal()
                }

                Flickable {
                    id: frame
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: true
                    contentWidth: column.width
                    contentHeight: column.height
                    flickableDirection: {
                        if (frame.contentHeight <= scrollView.height)
                            return Flickable.HorizontalFlick

                        if (frame.contentWidth <= scrollView.width)
                            return Flickable.VerticalFlick

                        return Flickable.HorizontalAndVerticalFlick
                    }

                    // ScrollView needs an extra TapHandler on top in order to receive click
                    // events. MouseAreas below ScrollView do not let clicks through.
                    TapHandler {
                        id: tapHandler
                        onTapped: root.focusSignal()
                    }

                    Column {
                        id: column

                        property bool hoverEnabled: false
                        onPositioningComplete: column.hoverEnabled = true

                        // Grid sizes
                        property int gridSpacing: 20
                        property int gridRowSpacing: 5
                        property int gridPadding: 5
                        property int col1Width: 100 // labels
                        property int col2Width: stateBackground.innerWidth - column.gridSpacing - 2
                                                * column.gridPadding - column.col1Width // controls

                        width: stateBackground.innerWidth
                        spacing: stateBackground.thumbSpacing

                        Repeater {
                            model: propertyChangesModel

                            delegate: Rectangle {
                                id: propertyChanges

                                required property int index

                                required property string target
                                required property bool explicit
                                required property bool restoreEntryValues
                                required property var propertyModelNode

                                width: column.width
                                height: propertyChangesColumn.height
                                color: StudioTheme.Values.themeBackgroundColorAlternate

                                PropertyModel {
                                    id: propertyModel
                                    modelNodeBackendProperty: propertyChanges.propertyModelNode
                                }

                                Column {
                                    id: propertyChangesColumn

                                    Item {
                                        id: section
                                        property int animationDuration: 120
                                        property bool expanded: propertyModel.expanded

                                        clip: true
                                        width: stateBackground.innerWidth
                                        height: Math.round(sectionColumn.height + header.height)

                                        Rectangle {
                                            id: header
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            width: stateBackground.innerWidth
                                            height: StudioTheme.Values.sectionHeadHeight
                                            color: StudioTheme.Values.themeSectionHeadBackground

                                            Row {
                                                x: column.gridPadding
                                                anchors.verticalCenter: parent.verticalCenter
                                                spacing: column.gridSpacing

                                                Text {
                                                    color: StudioTheme.Values.themeTextColor
                                                    text: qsTr("Target")
                                                    font.pixelSize: StudioTheme.Values.baseFontSize
                                                    horizontalAlignment: Text.AlignRight
                                                    width: column.col1Width
                                                }

                                                Text {
                                                    color: StudioTheme.Values.themeTextColor
                                                    text: propertyChanges.target
                                                    font.pixelSize: StudioTheme.Values.baseFontSize
                                                    horizontalAlignment: Text.AlignLeft
                                                    elide: Text.ElideRight
                                                    width: column.col2Width
                                                }
                                            }

                                            Text {
                                                id: arrow
                                                width: StudioTheme.Values.spinControlIconSizeMulti
                                                height: StudioTheme.Values.spinControlIconSizeMulti
                                                text: StudioTheme.Constants.sectionToggle
                                                color: StudioTheme.Values.themeTextColor
                                                renderType: Text.NativeRendering
                                                anchors.left: parent.left
                                                anchors.leftMargin: 4
                                                anchors.verticalCenter: parent.verticalCenter
                                                font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
                                                font.family: StudioTheme.Constants.iconFont.family

                                                Behavior on rotation {
                                                    NumberAnimation {
                                                        easing.type: Easing.OutCubic
                                                        duration: section.animationDuration
                                                    }
                                                }
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                onClicked: {
                                                    section.expanded = !section.expanded
                                                    propertyModel.setExpanded(section.expanded)
                                                    if (!section.expanded)
                                                        section.forceActiveFocus()
                                                    root.focusSignal()
                                                }
                                            }
                                        }

                                        Column {
                                            id: sectionColumn
                                            y: header.height
                                            padding: column.gridPadding
                                            spacing: column.gridRowSpacing

                                            Row {
                                                spacing: column.gridSpacing

                                                Text {
                                                    color: StudioTheme.Values.themeTextColor
                                                    text: qsTr("Explicit")
                                                    font.pixelSize: StudioTheme.Values.baseFontSize
                                                    horizontalAlignment: Text.AlignRight
                                                    verticalAlignment: Text.AlignVCenter
                                                    width: column.col1Width
                                                    height: explicitSwitch.height
                                                }

                                                StudioControls.Switch {
                                                    id: explicitSwitch
                                                    actionIndicatorVisible: false
                                                    checked: propertyChanges.explicit
                                                    onToggled: {
                                                        root.focusSignal()
                                                        propertyModel.setExplicit(
                                                                    explicitSwitch.checked)
                                                    }
                                                }
                                            }

                                            Row {
                                                spacing: column.gridSpacing

                                                Text {
                                                    color: StudioTheme.Values.themeTextColor
                                                    text: qsTr("Restore values")
                                                    font.pixelSize: StudioTheme.Values.baseFontSize
                                                    horizontalAlignment: Text.AlignRight
                                                    verticalAlignment: Text.AlignVCenter
                                                    width: column.col1Width
                                                    height: restoreSwitch.height
                                                }

                                                StudioControls.Switch {
                                                    id: restoreSwitch
                                                    actionIndicatorVisible: false
                                                    checked: propertyChanges.restoreEntryValues
                                                    onToggled: {
                                                        root.focusSignal()
                                                        propertyModel.setRestoreEntryValues(
                                                                    restoreSwitch.checked)
                                                    }
                                                }
                                            }
                                        }

                                        Behavior on height {
                                            NumberAnimation {
                                                easing.type: Easing.OutCubic
                                                duration: section.animationDuration
                                            }
                                        }

                                        states: [
                                            State {
                                                name: "Expanded"
                                                when: section.expanded
                                                PropertyChanges {
                                                    target: arrow
                                                    rotation: 0
                                                }
                                            },
                                            State {
                                                name: "Collapsed"
                                                when: !section.expanded
                                                PropertyChanges {
                                                    target: section
                                                    height: header.height
                                                }
                                                PropertyChanges {
                                                    target: arrow
                                                    rotation: -90
                                                }
                                            }
                                        ]
                                    }

                                    Column {
                                        id: propertyColumn
                                        padding: column.gridPadding
                                        spacing: column.gridRowSpacing

                                        Repeater {
                                            model: propertyModel

                                            onModelChanged: column.hoverEnabled = false

                                            delegate: ItemDelegate {
                                                id: propertyDelegate

                                                required property string name
                                                required property var value
                                                required property string type

                                                width: stateBackground.innerWidth - 2 * column.gridPadding
                                                height: 26
                                                hoverEnabled: column.hoverEnabled

                                                onClicked: root.focusSignal()

                                                background: Rectangle {
                                                    color: "transparent"
                                                    border.color: StudioTheme.Values.themeInteraction
                                                    border.width: propertyDelegate.hovered ? StudioTheme.Values.border : 0
                                                }

                                                Item {
                                                    id: removeItem
                                                    visible: propertyDelegate.hovered
                                                    x: propertyDelegate.width - removeItem.width
                                                    z: 10
                                                    width: removeItem.visible ? propertyDelegate.height : 0
                                                    height: removeItem.visible ? propertyDelegate.height : 0

                                                    Label {
                                                        id: removeIcon
                                                        anchors.fill: parent
                                                        text: StudioTheme.Constants.closeCross
                                                        color: StudioTheme.Values.themeTextColor
                                                        font.family: StudioTheme.Constants.iconFont.family
                                                        font.pixelSize: StudioTheme.Values.myIconFontSize
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignHCenter
                                                        scale: propertyDelegateMouseArea.containsMouse ? 1.2 : 1.0
                                                    }

                                                    MouseArea {
                                                        id: propertyDelegateMouseArea
                                                        anchors.fill: parent
                                                        hoverEnabled: column.hoverEnabled
                                                        onClicked: {
                                                            root.focusSignal()
                                                            propertyModel.removeProperty(
                                                                        propertyDelegate.name)
                                                        }
                                                    }
                                                }

                                                Row {
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    spacing: column.gridSpacing

                                                    Text {
                                                        color: StudioTheme.Values.themeTextColor
                                                        text: propertyDelegate.name
                                                        font.pixelSize: StudioTheme.Values.baseFontSize
                                                        horizontalAlignment: Text.AlignRight
                                                        elide: Text.ElideRight
                                                        width: column.col1Width
                                                    }

                                                    Text {
                                                        color: StudioTheme.Values.themeTextColor
                                                        text: propertyDelegate.value
                                                        font.pixelSize: StudioTheme.Values.baseFontSize
                                                        horizontalAlignment: Text.AlignLeft
                                                        elide: Text.ElideRight
                                                        width: column.col2Width - removeItem.width
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            BindingEditor {
                id: bindingEditor

                property string newWhenCondition

                property Timer timer: Timer {
                    id: timer
                    running: false
                    interval: 50
                    repeat: false
                    onTriggered: {
                        if (whenCondition.previousCondition === bindingEditor.newWhenCondition)
                            return

                        if ( bindingEditor.newWhenCondition !== "")
                            StatesEditorBackend.statesEditorModel.setWhenCondition(root.internalNodeId,
                                                               bindingEditor.newWhenCondition)
                        else
                            StatesEditorBackend.statesEditorModel.resetWhenCondition(root.internalNodeId)
                    }
                }

                stateModelNodeProperty: StatesEditorBackend.statesEditorModel.stateModelNode(root.internalNodeId)
                stateNameProperty: root.stateName

                onRejected: bindingEditor.hideWidget()
                onAccepted: {
                    bindingEditor.newWhenCondition = bindingEditor.text.trim()
                    timer.start()
                    bindingEditor.hideWidget()
                }
            }

            StudioControls.TextField {
                id: whenCondition
                style: StudioTheme.Values.statesControlStyle

                property string previousCondition

                width: stateBackground.innerWidth
                height: stateBackground.controlHeight

                visible: !root.baseState
                indicatorVisible: true
                indicator.icon.text: StudioTheme.Constants.edit_medium
                indicator.onClicked: {
                    whenCondition.previousCondition = whenCondition.text

                    bindingEditor.showWidget()
                    bindingEditor.text = whenCondition.text
                    bindingEditor.prepareBindings()
                    bindingEditor.updateWindowName()
                }

                actionIndicatorVisible: false
                translationIndicatorVisible: false
                placeholderText: qsTr("When Condition")

                onActiveFocusChanged: {
                    if (whenCondition.activeFocus)
                        root.focusSignal()
                }

                onEditingFinished: {
                    // The check for contenxtMenuAboutToShow is necessary in order to make a the
                    // popup stay open if the when condition was changed. Otherwise editingFinished
                    // will be called and the model will be reset. The popup will trigger a focus
                    // change and editingFinished is triggered.
                    if (whenCondition.previousCondition === whenCondition.text
                        || whenCondition.contextMenuAboutToShow)
                        return

                    whenCondition.previousCondition = whenCondition.text

                    if (whenCondition.text !== "")
                        StatesEditorBackend.statesEditorModel.setWhenCondition(root.internalNodeId, root.whenCondition)
                    else
                        StatesEditorBackend.statesEditorModel.resetWhenCondition(root.internalNodeId)

                }

                Component.onCompleted: whenCondition.previousCondition = whenCondition.text
            }
        }

        MenuButton {
            id: menuButton
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            visible: !root.baseState
            checked: stateMenu.opened

            onPressed: {
                if (!stateMenu.opened)
                    stateMenu.popup()

                root.focusSignal()
            }
        }
    }

    StateMenu {
        id: stateMenu
        x: 56

        isBaseState: root.baseState
        isTiny: root.isTiny
        hasExtend: root.hasExtend
        propertyChangesVisible: root.propertyChangesVisible
        hasAnnotation: root.checkAnnotation()
        hasWhenCondition: root.hasWhenCondition

        onClone: root.clone()
        onExtend: root.extend()
        onRemove: root.remove()
        onToggle: root.setPropertyChangesVisible(!root.propertyChangesVisible)
        onResetWhenCondition: statesEditorModel.resetWhenCondition(root.internalNodeId)

        onJumpToCode: StatesEditorBackend.statesEditorModel.jumpToCode()

        onEditAnnotation: {
            StatesEditorBackend.statesEditorModel.setAnnotation(root.internalNodeId)
            stateMenu.hasAnnotation = root.checkAnnotation()
        }
        onRemoveAnnotation: {
            StatesEditorBackend.statesEditorModel.removeAnnotation(root.internalNodeId)
            stateMenu.hasAnnotation = root.checkAnnotation()
        }

        onOpened: stateMenu.hasAnnotation = root.checkAnnotation()
    }

    property bool anyControlHovered: defaultButton.hovered || menuButton.hovered
                                     || scrollView.hovered || stateNameField.hover
                                     || whenCondition.hover

    states: [
        State {
            name: "default"
            when: !mouseArea.containsMouse && !root.anyControlHovered && !dragHandler.active
                  && !root.baseState

            PropertyChanges {
                target: stateBackground
                color: StudioTheme.Values.themeToolbarBackground
            }
        },
        State {
            name: "hover"
            when: (mouseArea.containsMouse || root.anyControlHovered) && !dragHandler.active

            PropertyChanges {
                target: stateBackground
                color: StudioTheme.Values.themeStateBackgroundColor_hover
            }
        },
        State {
            name: "baseState"
            when: root.baseState && !mouseArea.containsMouse && !root.anyControlHovered
                  && !dragHandler.active

            PropertyChanges {
                target: stateBackground
                color: StudioTheme.Values.themeThumbnailBackground_baseState
            }
        },
        State {
            name: "drag"
            when: dragHandler.active

            ParentChange {
                target: root
                parent: root.dragParent
            }

            AnchorChanges {
                target: root
                anchors.horizontalCenter: undefined
                anchors.verticalCenter: undefined
            }
        }
    ]
}
