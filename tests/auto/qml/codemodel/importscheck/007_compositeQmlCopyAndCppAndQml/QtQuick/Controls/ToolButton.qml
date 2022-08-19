// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ToolButton
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief Provides a button type that is typically used within a ToolBar.

     ToolButton is functionally similar to \l Button, but can provide a look that is more
     suitable within a \l ToolBar.

     \code
     ToolButton {
        iconSource: "edit-cut.png"
     }
     \endcode

    You can create a custom appearance for a ToolButton by
    assigning a \l {QtQuick.Controls.Styles::ButtonStyle}{ButtonStyle}.
*/

Button {
    id: button
    style: Qt.createComponent(Settings.style + "/ToolButtonStyle.qml", button)
}
