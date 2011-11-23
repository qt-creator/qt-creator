/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.0
import "widgets"

Rectangle {
    id: rectangle1
    width: 1024
    height: Math.min(920, parent.height - y)

    PageCaption {
        id: pageCaption

        x: 32
        y: 8

        anchors.rightMargin: 16
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.left: parent.left

        caption: qsTr("Examples")
    }

    CustomScrollArea {
        id: scrollArea;

        anchors.rightMargin: 38
        anchors.bottomMargin: 60
        anchors.leftMargin: 38
        anchors.topMargin: 102
        anchors.fill: parent

        CustomizedGridView {
            model: examplesModel
        }
    }

    SearchBar {
        id: searchBar

        y: 60

        anchors.right: parent.right
        anchors.rightMargin: 60
        anchors.left: parent.left
        anchors.leftMargin: 60

        placeholderText: qsTr("Search in Examples...")
        onTextChanged: examplesModel.parseSearchString(text)
    }

    Rectangle {
        id: gradiant
        y: 102
        height: 10
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#00ffffff"
            }
        }

        anchors.left: parent.left
        anchors.leftMargin: 38
        anchors.right: parent.right
        anchors.rightMargin: 38
    }

    Rectangle {
        id: gradiant1
        x: 38
        y: 570
        height: 10
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#00ffffff"
            }

            GradientStop {
                position: 1
                color: "#ffffff"
            }
        }

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        anchors.rightMargin: 38
        anchors.right: parent.right
        anchors.leftMargin: 38
        anchors.left: parent.left
    }
}
