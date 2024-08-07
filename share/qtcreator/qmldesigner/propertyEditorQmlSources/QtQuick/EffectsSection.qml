// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import HelperWidgets
import StudioTheme as StudioTheme
import StudioControls as StudioControls

Section {
    id: root

    property bool hasDesignerEffect: false
    property var model
    property var effectNode
    property var effectNodeWrapper

    // Draggging
    property Item draggedSec: null
    property var secsY: []
    property int moveFromIdx: 0
    property int moveToIdx: 0

    function invalidate() {
        root.effectNode = null
        root.model = null

        var effect = modelNodeBackend.allChildrenOfType("DesignEffect")
        root.effectNode = effect
        root.effectNodeWrapper = modelNodeBackend.registerSubSelectionWrapper(effect)
        root.hasDesignerEffect = effect.length === 1

        if (!root.hasDesignerEffect)
            return

        root.model = modelNodeBackend.allChildren(effect[0]) // ids for all effects
    }

    leftPadding: 0
    anchors.left: parent.left
    anchors.right: parent.right
    caption: qsTr('Effects <a style="color:%1;">[beta]</a>').arg(StudioTheme.Values.themeInteraction)
    visible: backendValues.layer_effect.isAvailable

    property Connections connection: Connections {
        target: modelNodeBackend

        function onSelectionChanged() { root.invalidate() }
        function onSelectionToBeChanged() { root.model = [] }
    }

    SectionLayout {
        x: StudioTheme.Values.sectionLeftPadding

        PropertyLabel {}

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            AbstractButton {
                id: effectButton
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: StudioTheme.Values.singleControlColumnWidth
                buttonIcon: root.hasDesignerEffect ? qsTr("Remove Effects") : qsTr("Add Effects")
                iconFont: StudioTheme.Constants.font
                tooltip: qsTr("Adds visual effects on the component.")
                onClicked: {
                    if (root.hasDesignerEffect) {
                         root.effectNodeWrapper.deleteModelNode()
                    } else {
                        modelNodeBackend.createModelNode(-1, "data", "DesignEffect", "QtQuick.Studio.DesignEffects")
                        var effectNode = modelNodeBackend.allChildrenOfType("DesignEffect")
                        modelNodeBackend.createModelNode(effectNode, "effects", "DesignDropShadow")
                    }
                    root.invalidate()
                }
            }

            ExpandingSpacer {}
        }

        PropertyLabel {
            text: qsTr("Visible")
            tooltip: qsTr("Toggles the visibility of visual effects on the component.")
            visible: root.hasDesignerEffect
        }

        SecondColumnLayout {
            visible: root.hasDesignerEffect

            CheckBox {
                implicitWidth: StudioTheme.Values.twoControlColumnWidth
                               + StudioTheme.Values.actionIndicatorWidth
                backendValue: root.effectNodeWrapper?.properties.visible
            }

            ExpandingSpacer {}
        }
    }

    Item {
        visible: root.hasDesignerEffect
        width: 1
        height: StudioTheme.Values.sectionHeadSpacerHeight
    }

    function handleDragMove() {
        root.dragTimer.stop()
        if (root.secsY.length === 0) {
            for (let i = 0; i < repeater.count; ++i)
                root.secsY[i] = repeater.itemAt(i).y
        }

        let scrollView = Controller.mainScrollView

        let oldContentY = scrollView.contentY
        if (root.draggedSec.y < scrollView.dragScrollMargin + scrollView.contentY
                && scrollView.contentY > 0) {
            scrollView.contentY -= scrollView.dragScrollMargin / 2
        } else if (root.draggedSec.y > scrollView.contentY + scrollView.height - scrollView.dragScrollMargin
                    && scrollView.contentY < scrollView.contentHeight - scrollView.height) {
            scrollView.contentY += scrollView.dragScrollMargin / 2
            if (scrollView.contentY > scrollView.contentHeight - scrollView.height)
                scrollView.contentY = scrollView.contentHeight - scrollView.height
        }

        if (scrollView.contentY < 0)
            scrollView.contentY = 0

        if (oldContentY !== scrollView.contentY) {
            // Changing dragged section position in drag handler doesn't seem to stick
            // when triggered by mouse move, so do it again async
            root.dragTimer.targetY = root.draggedSec.y - oldContentY + scrollView.contentY
            root.dragTimer.restart()
            root.dragConnection.enabled = false
            root.draggedSec.y = root.dragTimer.targetY
            root.dragConnection.enabled = true
        }

        root.moveToIdx = root.moveFromIdx
        for (let i = 0; i < repeater.count; ++i) {
            let currItem = repeater.itemAt(i)
            if (i > root.moveFromIdx) {
                if (root.draggedSec.y > currItem.y) {
                    currItem.y = root.secsY[i] - root.draggedSec.height - nodesCol.spacing
                    root.moveToIdx = i
                } else {
                    currItem.y = root.secsY[i]
                }
            } else if (i < root.moveFromIdx) {
                if (root.draggedSec.y < currItem.y) {
                    currItem.y = root.secsY[i] + root.draggedSec.height + nodesCol.spacing
                    root.moveToIdx = Math.min(root.moveToIdx, i)
                } else {
                    currItem.y = root.secsY[i]
                }
            }
        }
    }

    property Connections dragConnection: Connections {
        target: root.draggedSec

        function onYChanged() { root.handleDragMove() }
    }

    property Timer dragTimer: Timer {
        running: false
        interval: 16
        repeat: false

        property real targetY: -1

        onTriggered: {
            // Ensure we get position change triggers even if user holds mouse still to
            // make scrolling smooth
            root.draggedSec.y = targetY
            root.handleDragMove()
        }
    }

    Column {
        id: nodesCol
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 1

        Section {
            sectionHeight: 37
            anchors.left: parent.left
            anchors.right: parent.right
            caption: qsTr("Layer Blur")
            labelCapitalization: Font.MixedCase
            visible: root.hasDesignerEffect
            category: "DesignEffects"
            expanded: false

            SectionLayout {

                PropertyLabel {
                    text: qsTr("Visible")
                    tooltip: qsTr("Toggles the visibility of the <b>Layer Blur</b> on the component.")
                }

                SecondColumnLayout {
                    CheckBox {
                        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                       + StudioTheme.Values.actionIndicatorWidth
                        backendValue: root.effectNodeWrapper?.properties.layerBlurVisible
                    }

                    ExpandingSpacer {}
                }

                PropertyLabel {
                    text: qsTr("Blur")
                    tooltip: qsTr("Sets the intensity of the <b>Layer Blur</b> on the component.")
                }

                SecondColumnLayout {
                    SpinBox {
                        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                       + StudioTheme.Values.actionIndicatorWidth
                        backendValue: root.effectNodeWrapper?.properties.layerBlurRadius
                        minimumValue: 0
                        maximumValue: 250
                    }

                    ExpandingSpacer {}
                }
            }
        }

        Section {
            sectionHeight: 37
            anchors.left: parent.left
            anchors.right: parent.right
            caption: qsTr("Background Blur")
            labelCapitalization: Font.MixedCase
            visible: root.hasDesignerEffect
            category: "DesignEffects"
            expanded: false

            SectionLayout {

                PropertyLabel {
                    text: qsTr("Visible")
                    tooltip: qsTr("Toggles the visibility of blur on the selected background component.")
                }

                SecondColumnLayout {
                    CheckBox {
                        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                       + StudioTheme.Values.actionIndicatorWidth
                        backendValue: root.effectNodeWrapper?.properties.backgroundBlurVisible
                    }

                    ExpandingSpacer {}
                }

                PropertyLabel {
                    text: qsTr("Blur")
                    tooltip: qsTr("Sets the intensity of blur on the selected background component.\n"
                            + "The foreground component should be transparent, and the background "
                            + "component should be opaque.")
                }

                SecondColumnLayout {
                    SpinBox {
                        implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                       + StudioTheme.Values.actionIndicatorWidth
                        backendValue: root.effectNodeWrapper?.properties.backgroundBlurRadius
                        minimumValue: 0
                        maximumValue: 250
                    }

                    ExpandingSpacer {}
                }

                PropertyLabel {
                    text: qsTr("Background")
                    tooltip: qsTr("Sets a component as the background of a transparent component."
                            + "The <b>Background Blur</b> works only on this component. The component should "
                            + "be solid.")
                }

                SecondColumnLayout {
                    ItemFilterComboBox {
                        implicitWidth: StudioTheme.Values.singleControlColumnWidth
                                       + StudioTheme.Values.actionIndicatorWidth
                        width: implicitWidth
                        typeFilter: "QtQuick.Item"
                        backendValue: root.effectNodeWrapper?.properties.backgroundLayer
                    }

                    ExpandingSpacer {}
                }
            }
        }

        Repeater {
            id: repeater
            model: root.model

            Section {
                id: delegate

                property QtObject wrapper: modelNodeBackend.registerSubSelectionWrapper(modelData)
                property bool wasExpanded: false

                Behavior on y {
                    id: dragAnimation

                    PropertyAnimation {
                        duration: 300
                        easing.type: Easing.InOutQuad
                    }
                }

                onStartDrag: function(section) {
                    root.draggedSec = section
                    root.moveFromIdx = index
                    // We only need to animate non-dragged sections
                    dragAnimation.enabled = false
                    delegate.wasExpanded = delegate.expanded
                    delegate.expanded = false
                    delegate.highlightBorder = true
                    root.secsY = []
                }

                onStopDrag: {
                    if (root.secsY.length !== 0) {
                        if (root.moveFromIdx === root.moveToIdx)
                            root.draggedSec.y = root.secsY[root.moveFromIdx]
                        else
                            modelNodeBackend.moveNode(root.effectNode, "effects", root.moveFromIdx, root.moveToIdx)
                    }

                    delegate.highlightBorder = false
                    root.draggedSec = null
                    delegate.expanded = delegate.wasExpanded
                    dragAnimation.enabled = true

                    Qt.callLater(root.invalidate)
                }

                sectionHeight: 37
                anchors.left: parent.left
                anchors.right: parent.right
                category: "DesignEffects"
                fillBackground: true
                expanded: false

                draggable: true
                showCloseButton: true

                content: StudioControls.ComboBox {
                    id: shadowComboBox
                    actionIndicatorVisible: false
                    width: 200
                    textRole: "text"
                    valueRole: "value"
                    model: [
                        { value: "DesignDropShadow", text: qsTr("Drop Shadow") },
                        { value: "DesignInnerShadow", text: qsTr("Inner Shadow") }
                    ]
                    anchors.verticalCenter: parent.verticalCenter

                    // When an item is selected, update the backend.
                    onActivated: {
                        delegate.wrapper.properties.showBehind.resetValue()
                        modelNodeBackend.changeType(modelData, shadowComboBox.currentValue)
                    }
                    // Set the initial currentIndex to the value stored in the backend.
                    Component.onCompleted: {
                        shadowComboBox.currentIndex = shadowComboBox.indexOfValue(modelNodeBackend.simplifiedTypeName(modelData))
                    }
                }

                onCloseButtonClicked: {
                    delegate.wrapper.deleteModelNode()
                    Qt.callLater(root.invalidate)
                }

                SectionLayout {
                    id: controlContainer
                    property bool isDropShadow: shadowComboBox.currentValue === "DesignDropShadow"

                    PropertyLabel {
                        text: qsTr("Visible")
                        tooltip: qsTr("Toggles the visibility of the component shadow.")
                    }

                    SecondColumnLayout {
                        CheckBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.visible
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel {
                        text: qsTr("Blur")
                        tooltip: qsTr("Sets the softness of the component shadow. A larger value"
                                + " causes the edges of the shadow to appear more blurry.")
                    }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.blur
                            minimumValue: 0
                            maximumValue: 250
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel {
                        text: qsTr("Spread")
                        tooltip: modelNodeBackend.isInstanceOf("Rectangle")
                                    ? qsTr("Resizes the base shadow of the component by pixels.")
                                    : qsTr("Only supported for Rectangles.")
                        enabled: modelNodeBackend.isInstanceOf("Rectangle")
                    }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.spread
                            enabled: modelNodeBackend.isInstanceOf("Rectangle")
                            minimumValue: -2048
                            maximumValue: 2048
                        }

                        ExpandingSpacer {}
                    }

                    PropertyLabel {
                        text: qsTr("Color")
                        tooltip: qsTr("Sets the color of the shadow.")
                    }

                    ColorEditor {
                        backendValue: delegate.wrapper.properties.color
                        supportGradient: false
                    }

                    PropertyLabel {
                        text: qsTr("Offset")
                        tooltip: qsTr("Moves the shadow with respect to the component in "
                                + "X and Y coordinates by pixels.")
                    }

                    SecondColumnLayout {
                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.offsetX
                            minimumValue: -0xffff
                            maximumValue: 0xffff
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel {
                            text: "X"
                            tooltip: qsTr("X-coordinate")
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlGap }

                        SpinBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.offsetY
                            minimumValue: -0xffff
                            maximumValue: 0xffff
                        }

                        Spacer { implicitWidth: StudioTheme.Values.controlLabelGap }

                        ControlLabel {
                            text: "Y"
                            tooltip: qsTr("Y-coordinate")
                        }

                        ExpandingSpacer {}
                    }


                    PropertyLabel {
                        visible: controlContainer.isDropShadow
                        text: qsTr("Show behind")
                        tooltip: qsTr("Toggles the visibility of the shadow behind a transparent component.")
                    }

                    SecondColumnLayout {
                        visible: controlContainer.isDropShadow

                        CheckBox {
                            implicitWidth: StudioTheme.Values.twoControlColumnWidth
                                           + StudioTheme.Values.actionIndicatorWidth
                            backendValue: delegate.wrapper.properties.showBehind
                        }

                        ExpandingSpacer {}
                    }
                }
            }
        }
    }

    Item {
        visible: root.hasDesignerEffect
        width: 1
        height: StudioTheme.Values.sectionHeadSpacerHeight
    }

    SectionLayout {
        x: StudioTheme.Values.sectionLeftPadding
        visible: root.hasDesignerEffect

        PropertyLabel {}

        SecondColumnLayout {
            Spacer { implicitWidth: StudioTheme.Values.actionIndicatorWidth }

            AbstractButton {
                id: addShadowEffectButton
                implicitWidth: StudioTheme.Values.singleControlColumnWidth
                width: StudioTheme.Values.singleControlColumnWidth
                buttonIcon: qsTr("Add Shadow Effect")
                iconFont: StudioTheme.Constants.font
                tooltip: qsTr("Adds <b>Drop Shadow</b> or <b>Inner Shadow</b> effects to a component.")
                onClicked: {
                    modelNodeBackend.createModelNode(root.effectNode,
                                                     "effects",
                                                     "DesignDropShadow")
                    root.invalidate()
                }
            }

            ExpandingSpacer {}
        }
    }
}
