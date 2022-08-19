// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1

/*!
        \qmltype StackViewSlideTransition
        \internal
        \inqmlmodule QtQuick.Controls.Private
*/
StackViewDelegate {
    id: root

    property bool horizontal: true

    function getTransition(properties)
    {
        return root[horizontal ? "horizontalSlide" : "verticalSlide"][properties.name]
    }

    function transitionFinished(properties)
    {
        properties.exitItem.x = 0
        properties.exitItem.y = 0
    }

    property QtObject horizontalSlide: QtObject {
        property Component pushTransition: StackViewTransition {
            PropertyAnimation {
                target: enterItem
                property: "x"
                from: target.width
                to: 0
                duration: 300
            }
            PropertyAnimation {
                target: exitItem
                property: "x"
                from: 0
                to: -target.width
                duration: 300
            }
        }

        property Component popTransition: StackViewTransition {
            PropertyAnimation {
                target: enterItem
                property: "x"
                from: -target.width
                to: 0
                duration: 300
            }
            PropertyAnimation {
                target: exitItem
                property: "x"
                from: 0
                to: target.width
                duration: 300
            }
        }
        property Component replaceTransition: pushTransition
    }

    property QtObject verticalSlide: QtObject {
        property Component pushTransition: StackViewTransition {
            PropertyAnimation {
                target: enterItem
                property: "y"
                from: target.height
                to: 0
                duration: 300
            }
            PropertyAnimation {
                target: exitItem
                property: "y"
                from: 0
                to: -target.height
                duration: 300
            }
        }

        property Component popTransition: StackViewTransition {
            PropertyAnimation {
                target: enterItem
                property: "y"
                from: -target.height
                to: 0
                duration: 300
            }
            PropertyAnimation {
                target: exitItem
                property: "y"
                from: 0
                to: target.height
                duration: 300
            }
            property Component replaceTransition: pushTransition
        }
    }
}
