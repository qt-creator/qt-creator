// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import QtQuick.Templates 2.15 as T
import StudioTheme 1.0 as StudioTheme

Row {
    id: root

    signal originSelectorClicked(string value)
    property alias origin: myButton.origin
    property variant backendValue

    onOriginSelectorClicked: function(value) {
        if (root.enabled)
            root.backendValue.setEnumeration("Item", value)
    }

    Connections {
        target: modelNodeBackend
        function onSelectionChanged() { originPopup.close() }
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: root.backendValue
    }

    ColorLogic {
        id: colorLogic
        backendValue: root.backendValue
        onValueFromBackendChanged: {
            var enumString = root.backendValue.enumeration
            if (enumString === "")
                enumString = root.backendValue.value

            root.origin = enumString === undefined ? "Center" : enumString
        }
    }

    ActionIndicator {
        id: actionIndicator
        __parentControl: myButton
        x: 0
        y: 0
        width: actionIndicator.visible ? myButton.__actionIndicatorWidth : 0
        height: actionIndicator.visible ? myButton.__actionIndicatorHeight : 0

        icon.color: extFuncLogic.color
        icon.text: extFuncLogic.glyph
        onClicked: extFuncLogic.show()
        forceVisible: extFuncLogic.menuVisible
    }

    T.AbstractButton {
        id: myButton

        function originSelectorClicked(value) { root.originSelectorClicked(value) }

        property string origin: "Center"

        // This property is used to indicate the global hover state
        property bool hover: myButton.hovered && root.enabled

        property alias backgroundVisible: buttonBackground.visible
        property alias backgroundRadius: buttonBackground.radius

        property alias actionIndicator: actionIndicator

        property alias actionIndicatorVisible: actionIndicator.visible
        property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
        property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)
        height: StudioTheme.Values.height
        width: StudioTheme.Values.height
        z: myButton.checked ? 10 : 3
        activeFocusOnTab: false

        onClicked: originPopup.opened ? originPopup.close() : originPopup.open()

        background: Rectangle {
            id: buttonBackground
            color: myButton.checked ? StudioTheme.Values.themeControlBackgroundInteraction
                                    : StudioTheme.Values.themeControlBackground
            border.color: myButton.checked ? StudioTheme.Values.themeInteraction
                                           : StudioTheme.Values.themeControlOutline
            border.width: StudioTheme.Values.border
        }

        indicator: OriginIndicator {
            myControl: myButton
            x: 0
            y: 0
            implicitWidth: myButton.width
            implicitHeight: myButton.height
        }

        T.Popup {
            id: originPopup

            x: 50
            y: 0

            width: (4 * grid.spacing) + (3 * StudioTheme.Values.height)
            height: (4 * grid.spacing) + (3 * StudioTheme.Values.height)

            padding: StudioTheme.Values.border
            margins: 0 // If not defined margin will be -1

            closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                         | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                         | T.Popup.CloseOnReleaseOutsideParent

            contentItem: Item {
                Grid {
                    id: grid

                    x: 5
                    y: 5

                    rows: 3
                    columns: 3
                    spacing: 5 // TODO spacingvalue in Values.qml

                    OriginSelector { myControl: myButton; value: "TopLeft" }
                    OriginSelector { myControl: myButton; value: "Top" }
                    OriginSelector { myControl: myButton; value: "TopRight" }
                    OriginSelector { myControl: myButton; value: "Left" }
                    OriginSelector { myControl: myButton; value: "Center" }
                    OriginSelector { myControl: myButton; value: "Right" }
                    OriginSelector { myControl: myButton; value: "BottomLeft" }
                    OriginSelector { myControl: myButton; value: "Bottom" }
                    OriginSelector { myControl: myButton; value: "BottomRight" }
                }
            }

            background: Rectangle {
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeInteraction
                border.width: StudioTheme.Values.border
            }

            enter: Transition {}
            exit: Transition {}
        }

        states: [
            State {
                name: "default"
                when: myButton.enabled && !myButton.hover && !actionIndicator.hover
                      && !myButton.pressed && !originPopup.opened
                PropertyChanges {
                    target: buttonBackground
                    color: StudioTheme.Values.themeControlBackground
                }
                PropertyChanges {
                    target: myButton
                    z: 3
                }
            },
            State {
                name: "globalHover"
                when: actionIndicator.hover && !myButton.pressed && !originPopup.opened
                      && root.enabled
                PropertyChanges {
                    target: buttonBackground
                    color: StudioTheme.Values.themeControlBackgroundGlobalHover
                    border.color: StudioTheme.Values.themeControlOutline
                }
            },
            State {
                name: "hover"
                when: myButton.hover && !actionIndicator.hover && !myButton.pressed
                      && !originPopup.opened && root.enabled
                PropertyChanges {
                    target: buttonBackground
                    color: StudioTheme.Values.themeControlBackgroundHover
                    border.color: StudioTheme.Values.themeControlOutline
                }
            },
            State {
                name: "press"
                when: myButton.hover && myButton.pressed && !originPopup.opened
                PropertyChanges {
                    target: buttonBackground
                    color: StudioTheme.Values.themeControlBackgroundInteraction
                    border.color: StudioTheme.Values.themeInteraction
                }
                PropertyChanges {
                    target: myButton
                    z: 10
                }
            },
            State {
                name: "disable"
                when: !myButton.enabled
                PropertyChanges {
                    target: buttonBackground
                    color: StudioTheme.Values.themeControlBackgroundDisabled
                    border.color: StudioTheme.Values.themeControlOutlineDisabled
                }
            }
        ]
    }
}
