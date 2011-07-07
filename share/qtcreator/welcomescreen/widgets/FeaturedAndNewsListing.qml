/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.0
import components 1.0 as Components

Item {
    InsetText {
        id: text
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top:  parent.top
        anchors.margins: 10
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Featured News")
//        mainColor: "#44A51C"
        mainColor: "#074C1C"
        font.bold: true
        font.pointSize: 16
    }

    ListModel {
        id: tempNewsModel
        ListElement { title: ""; description: "Loading news sources..." ; blogIcon: ""; blogName: ""; link: "" }
    }

    NewsListing {
        id: newsList
        model: {
            if (aggregatedFeedsModel.articleCount > 0)
                return aggregatedFeedsModel
            else
                return tempNewsModel
        }
        anchors.bottom: parent.bottom
        anchors.top: text.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: text.height
        clip: true

    }

}
