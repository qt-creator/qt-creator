// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

HelperWidgets.Section {
    id: root

    caption: model.nodeName
    category: "EffectMaker"

//    TODO: implement effect properties
//    property var propList: model.props

    Column {
        anchors.fill: parent
        spacing: 2

    //    Repeater {
    //        id: effects
    //        model: effectList
    //        width: parent.width
    //        height: parent.height
    //        delegate: Text {
    //            width: parent.width
    //            //height: StudioTheme.Values.checkIndicatorHeight * 2 // TODO: update or remove
    //        }
    //    }
    }
}

