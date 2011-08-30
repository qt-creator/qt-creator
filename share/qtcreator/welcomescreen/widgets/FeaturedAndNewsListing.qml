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
import qtcomponents 1.0 as Components

Item {
    InsetText {
        id: text
        anchors.left: parent.left
        anchors.top:  parent.top
        anchors.margins: 14
        horizontalAlignment: Text.AlignLeft
        text: qsTr("Featured News")
        mainColor: "#074C1C"
        font.bold: true
        font.pointSize: 18
    }

    ListModel {
        id: fallbackNewsModel
        ListElement {
            title: "Welcome to Qt Creator 2.3";
            blogName: "The Qt Creator Team"
            description: "<div>This release adds lots of new features as well as a great amount of bug fixes:</div><ul><li>Example and tutorial browsing with descriptive texts, and filtering for examples matching a keyword</li><li>Enhanced C++ coding style options, with indent settings and alignment settings split up for the different use cases depending on element, including preview and separation between global and project specific settings</li><li>Support for deployment and running to a more general &#8220;remote Linux&#8221;</li><li>Support for compiling projects with the Clang compiler</li><li>Code completion doesn't block the editor any more</li><li>Profiling now has it&#8217;s own &#8220;Analyze&#8221; mode.</li><li>Symbian got CODA support, allowing for deployment via WiFi</li><li>Support for models and delegates in the Qt Quick Designer</li><li>Support for editing inline components and delegates</li><li>Improved Live Preview (a.k.a. modifying QML while the app is running in the debugger)</li><li>Added &#8216;Find usages&#8217; functionality for QML types</li></ul>"
            link: ""
        }
    }

    NewsListing {
        id: newsList
        model: {
            if (aggregatedFeedsModel.articleCount > 0)
                return aggregatedFeedsModel
            else {
                return fallbackNewsModel
            }
        }
        anchors.bottom: parent.bottom
        anchors.top: text.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
    }

}
