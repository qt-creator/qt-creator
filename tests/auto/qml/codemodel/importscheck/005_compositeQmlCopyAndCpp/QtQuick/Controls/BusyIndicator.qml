// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype BusyIndicator
    \inqmlmodule QtQuick.Controls
    \since 5.2
    \ingroup controls
    \brief A busy indicator.

    The busy indicator should be used to indicate activity while content is
    being loaded or the UI is blocked waiting for a resource to become available.

    You can create a custom appearance for a Busy Indicator by
    assigning a \l {QtQuick.Controls.Styles::BusyIndicatorStyle}{BusyIndicatorStyle}.
 */
Control {
    id: indicator

    /*! \qmlproperty bool BusyIndicator::running

    This property holds whether the busy indicator is currently indicating
    activity.

    \note The indicator is only visible when this property is set to \c true.

    The default value is \c true.
    */
    property bool running: true

    Accessible.role: Accessible.Indicator
    Accessible.name: "busy"

    style: Qt.createComponent(Settings.style + "/BusyIndicatorStyle.qml", indicator)
}
