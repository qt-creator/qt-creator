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

import QtQuick 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0

PropertyEditorPane {
    id: itemPane

    signal toolBarAction(int action)
    signal previewEnvChanged(string env)
    signal previewModelChanged(string model)

    // invoked from C++ to refresh material preview image
    function refreshPreview()
    {
        topSection.refreshPreview()
    }

    // Called also from C++ to close context menu on focus out
    function closeContextMenu()
    {
        topSection.closeContextMenu()
    }

    // Called from C++ to initialize preview menu checkmarks
    function initPreviewData(env, model)
    {
        topSection.previewEnv = env;
        topSection.previewModel = model
    }

    MaterialEditorTopSection {
        id: topSection

        onToolBarAction: (action) => itemPane.toolBarAction(action)
        onPreviewEnvChanged: itemPane.previewEnvChanged(previewEnv)
        onPreviewModelChanged: itemPane.previewModelChanged(previewModel)
    }

    Item { width: 1; height: 10 }

    DynamicPropertiesSection {
        propertiesModel: MaterialEditorDynamicPropertiesModel {}
    }

    Loader {
        id: specificsTwo

        property string theSource: specificQmlData

        anchors.left: parent.left
        anchors.right: parent.right
        visible: theSource !== ""
        sourceComponent: specificQmlComponent

        onTheSourceChanged: {
            active = false
            active = true
        }
    }

    Item {
        width: 1
        height: 10
        visible: specificsTwo.visible
    }

    Loader {
        id: specificsOne
        anchors.left: parent.left
        anchors.right: parent.right
        source: specificsUrl
    }
}
