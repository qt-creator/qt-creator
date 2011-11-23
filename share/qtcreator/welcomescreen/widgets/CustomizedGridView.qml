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

GridView {
    interactive: false
    width: scrollArea.width
    cellHeight: 240
    cellWidth: 216
    property int columns:  Math.max(Math.floor(width / cellWidth), 1)
    height: 240 * Math.ceil(count / columns)

    x: Math.max((width - (cellWidth * columns)) / 2, 0);

    delegate: Delegate {
        id: delegate

        property bool isHelpImage: model.imageUrl.search(/qthelp/)  != -1
        property string sourcePrefix: isHelpImage ? "image://helpimage/" : ""

        property string mockupSource: model.imageSource
        property string helpSource: model.imageUrl !== "" ? sourcePrefix + encodeURI(model.imageUrl) : ""

        imageSource: model.imageSource === undefined ? helpSource : mockupSource
        videoSource: model.imageSource === undefined ? model.imageUrl : mockupSource

        caption: model.name;
        description: model.description
        isVideo: model.isVideo === true
        videoLength: model.videoLength !== undefined ? model.videoLength : ""
        tags: model.tags
    }

}
