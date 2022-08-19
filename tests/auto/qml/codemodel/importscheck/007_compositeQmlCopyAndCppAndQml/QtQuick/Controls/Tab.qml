// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1

/*!
    \qmltype Tab
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup viewaddons
    \brief Tab represents the content of a tab in a TabView.

    A Tab item inherits from Loader and provides a similar
    api.
*/

Loader {
    id: tab
    anchors.fill: parent

    /*! This property holds the title of the tab. */
    property string title

    /*! \internal */
    property bool __inserted: false

    Accessible.role: Accessible.LayeredPane
    active: false
    visible: false

    activeFocusOnTab: false

    onVisibleChanged: if (visible) active = true

    /*! \internal */
    default property alias component: tab.sourceComponent
}
