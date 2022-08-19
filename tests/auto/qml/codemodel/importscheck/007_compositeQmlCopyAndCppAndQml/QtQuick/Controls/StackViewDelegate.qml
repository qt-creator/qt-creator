// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1

/*!
        \qmltype StackViewDelegate
        \inqmlmodule QtQuick.Controls
        \since 5.1

        \brief A delegate used by StackView for loading transitions.

        See the documentation for the \l {QtQuick.Controls1::StackView} {StackView}
        component.

*/
QtObject {
    id: root

    function getTransition(properties)
    {
        return root[properties.name]
    }

    function transitionFinished(properties)
    {
    }

    property Component pushTransition: StackViewTransition {}
    property Component popTransition: root["pushTransition"]
    property Component replaceTransition: root["pushTransition"]
}
