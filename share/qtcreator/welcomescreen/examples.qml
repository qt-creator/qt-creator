/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.1
import widgets 1.0

Item {

    anchors.fill: parent

    ComboBox {
        id: comboBox

        anchors.verticalCenter: searchBar.verticalCenter

        width: 200
        anchors.leftMargin: 30
        anchors.left: parent.left
        model: exampleSetModel
        textRole: "text"

        onCurrentIndexChanged: {
            if (comboBox.model === undefined)
                return;

            examplesModel.filterForExampleSet(currentIndex)
        }

        property int theIndex: examplesModel.exampleSetIndex

        onTheIndexChanged: {
            if (comboBox.model === undefined)
                return;
            if (theIndex != currentIndex)
                currentIndex = theIndex;
        }
    }

    SearchBar {
        id: searchBar

        y: screenDependHeightDistance
        anchors.left: comboBox.right
        anchors.leftMargin: 18
        anchors.rightMargin: 20
        anchors.right: parent.right

        placeholderText: qsTr("Search in Examples...")
        onTextChanged: examplesModel.parseSearchString(text)
    }

    CustomizedGridView {
        id: grid

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: screenDependHeightDistance

        model: examplesModel
    }
}
