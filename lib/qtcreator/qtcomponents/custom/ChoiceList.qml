/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0
import "./private" as Private

Item {
    id: choiceList

    property alias model: popup.model
    property alias currentIndex: popup.currentIndex
    property alias currentText: popup.currentText
    property alias popupOpen: popup.popupOpen
    property alias containsMouse: popup.containsMouse
    property alias pressed: popup.buttonPressed

    property Component background: null
    property Item backgroundItem: backgroundLoader.item
    property Component listItem: null
    property Component popupFrame: null

    property int leftMargin: 0
    property int topMargin: 0
    property int rightMargin: 0
    property int bottomMargin: 0

    property string popupBehavior
    width: 0
    height: 0

    property bool activeFocusOnPress: true

    Loader {
        id: backgroundLoader
        property alias styledItem: choiceList
        sourceComponent: background
        anchors.fill: parent
        property string currentItemText: model.get(currentIndex).text
    }

    Private.ChoiceListPopup {
        // NB: This ChoiceListPopup is also the mouse area
        // for the component (to enable drag'n'release)
        id: popup
        listItem: choiceList.listItem
        popupFrame: choiceList.popupFrame
    }

    Keys.onSpacePressed: { choiceList.popupOpen = !choiceList.popupOpen }
    Keys.onUpPressed: { if (currentIndex < model.count - 1) currentIndex++ }
    Keys.onDownPressed: {if (currentIndex > 0) currentIndex-- }
}
