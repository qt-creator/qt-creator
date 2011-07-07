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
import Monitor 1.0
import "MainView.js" as Plotter

BorderImage {
    id: rangeDetails

    property string duration    //###int?
    property string label
    property string type
    property string file
    property int line

    source: "popup.png"
    border {
        left: 10; top: 10
        right: 20; bottom: 20
    }

    width: col.width + 45
    height: childrenRect.height + 30
    z: 1
    visible: false

    //title
    Text {
        id: typeTitle
        text: rangeDetails.type
        font.bold: true
        y: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -5
    }

    //details
    Column {
        id: col
        anchors.top: typeTitle.bottom
        x: 2
        Detail {
            label: "Duration"
            content: rangeDetails.duration < 1000 ?
                        rangeDetails.duration + "μs" :
                        Math.floor(rangeDetails.duration/10)/100 + "ms"
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: "Details"
            content: {
                var inputString = rangeDetails.label;
                if (inputString.length > 7 && inputString.substring(0,7) == "file://") {
                    var pos = inputString.lastIndexOf("/");
                    return inputString.substr(pos+1);
                }
                // transform code blocks into oneliners
                inputString = inputString.replace("\n", " ");

                var maxLen = 40;
                if (inputString.length > maxLen)
                    inputString = inputString.substring(0,maxLen)+"...";

                return inputString;
            }
        }
        Detail {
            opacity: content.length !== 0 ? 1 : 0
            label: "Location"
            content: {
                var file = rangeDetails.file
                var pos = file.lastIndexOf("/")
                if (pos != -1)
                    file = file.substr(pos+1)
                return (file.length !== 0) ? (file + ":" + rangeDetails.line) : "";
            }
            onLinkActivated: Qt.openUrlExternally(url)
        }
    }
}
