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
import StudioTheme as StudioTheme
import StudioControls as StudioControls
import QtQuick.Layouts

StudioControls.Menu {
    id: root

    property bool isBaseState: false
    property bool isTiny: false
    property bool hasExtend: false
    property bool propertyChangesVisible: false
    property bool hasAnnotation: false
    property bool hasWhenCondition: false

    signal clone()
    signal extend()
    signal remove()
    signal toggle()
    signal resetWhenCondition()
    signal jumpToCode()
    signal editAnnotation()
    signal removeAnnotation()

    closePolicy: Popup.CloseOnReleaseOutside | Popup.CloseOnEscape

    StudioControls.MenuItem {
        id: clone
        visible: !root.isBaseState
        text: qsTr("Clone")
        height: clone.visible ? clone.implicitHeight : 0
        onTriggered: root.clone()
    }

    StudioControls.MenuItem {
        id: deleteState
        visible: !root.isBaseState
        text: qsTr("Delete")
        height: deleteState.visible ? deleteState.implicitHeight : 0
        onTriggered: root.remove()
    }

    StudioControls.MenuItem {
        id: showChanges
        visible: !root.isBaseState
        enabled: !root.isTiny
        text: root.propertyChangesVisible ? qsTr("Show Thumbnail") : qsTr("Show Changes")
        height: showChanges.visible ? showChanges.implicitHeight : 0
        onTriggered: root.toggle()
    }

    StudioControls.MenuItem {
        id: extend
        visible: !root.isBaseState && !root.hasExtend
        text: qsTr("Extend")
        height: extend.visible ? extend.implicitHeight : 0
        onTriggered: root.extend()
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        enabled: !root.isBaseState
        text: qsTr("Jump to the code")
        onTriggered: root.jumpToCode()
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        enabled: !root.isBaseState && root.hasWhenCondition
        text: qsTr("Reset when Condition")
        onTriggered: root.resetWhenCondition()
    }

    StudioControls.MenuSeparator {}

    StudioControls.MenuItem {
        enabled: !root.isBaseState
        text: root.hasAnnotation ? qsTr("Edit Annotation") : qsTr("Add Annotation")
        onTriggered: root.editAnnotation()
    }

    StudioControls.MenuItem {
        enabled: !isBaseState && hasAnnotation
        text: qsTr("Remove Annotation")
        onTriggered: root.removeAnnotation()
    }
}
