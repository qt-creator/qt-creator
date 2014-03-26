/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1
import QtQuick.Controls 1.0

GridView {
    x: Math.max((width - (cellWidth * columns)) / 2, 0);
    id: gridView
    interactive: false
    cellHeight: 240
    cellWidth: 216
    property int columns:  Math.max(Math.floor(width / cellWidth), 1)

    delegate: Loader {
        property int delegateOffset: cellHeight * Math.floor(index / columns) + 100
        property bool isVisible: delegateOffset > scrollView.flickableItem.contentY - cellHeight
                                 && delegateOffset < scrollView.flickableItem.contentY + scrollView.flickableItem.height
        onIsVisibleChanged: active = true
        visible: isVisible
        active: false
        sourceComponent: Delegate {
            id: delegate

            property bool isHelpImage: model.imageUrl.search(/qthelp/)  != -1
            property string sourcePrefix: isHelpImage ? "image://helpimage/" : ""

            property string mockupSource: model.imageSource
            property string helpSource: (model.imageUrl !== "" && model.imageUrl !== undefined) ? sourcePrefix + encodeURI(model.imageUrl) : ""

            imageSource: isVideo ? "" : (model.imageSource === undefined ? delegate.helpSource : delegate.mockupSource)

            videoSource: isVideo ? (model.imageSource === undefined ? model.imageUrl : mockupSource) : ""

            caption: model.name;
            description: model.description
            isVideo: model.isVideo === true
            videoLength: model.videoLength !== undefined ? model.videoLength : ""
            tags: model.tags
        }
    }
}
