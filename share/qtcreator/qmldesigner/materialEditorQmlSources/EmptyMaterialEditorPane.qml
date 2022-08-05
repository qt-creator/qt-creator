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
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioTheme 1.0 as StudioTheme

PropertyEditorPane {
    id: root

    signal toolBarAction(int action)
    signal previewEnvChanged(string env)
    signal previewModelChanged(string model)

    // Called from C++, dummy methods to avoid warnings
    function closeContextMenu() {}
    function initPreviewData(env, model) {}

    Column {
        id: col

        MaterialEditorToolBar {
            width: root.width

            onToolBarAction: (action) => root.toolBarAction(action)
        }

        Item {
            width: root.width - 2 * col.padding
            height: 150

            Text {
                text: hasQuick3DImport ? qsTr("There are no materials in this project.<br>Select '<b>+</b>' to create one.")
                                       : qsTr("To use <b>Material Editor</b>, first add the QtQuick3D module in the <b>Components</b> view.")
                textFormat: Text.RichText
                color: StudioTheme.Values.themeTextColor
                font.pixelSize: StudioTheme.Values.mediumFontSize
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                width: root.width
                anchors.centerIn: parent
            }
        }
    }
}
