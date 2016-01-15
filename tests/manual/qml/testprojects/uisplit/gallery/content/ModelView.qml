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

import QtQuick 2.2
import QtQuick.Controls 1.2
//import QtQuick.XmlListModel 2.1

Item {
    id: root

    width: 600
    height: 300

    //    XmlListModel {
    //        id: flickerModel
    //        source: "http://api.flickr.com/services/feeds/photos_public.gne?format=rss2&tags=" + "Cat"
    //        query: "/rss/channel/item"
    //        namespaceDeclarations: "declare namespace media=\"http://search.yahoo.com/mrss/\";"
    //        XmlRole { name: "title"; query: "title/string()" }
    //        XmlRole { name: "imagesource"; query: "media:thumbnail/@url/string()" }
    //        XmlRole { name: "credit"; query: "media:credit/string()" }
    //    }

    TableView{
        model: DummyModel {

        }

        anchors.fill: parent

        TableViewColumn {
            role: "index"
            title: "#"
            width: 36
            resizable: false
            movable: false
        }
        TableViewColumn {
            role: "title"
            title: "Title"
            width: 120
        }
        TableViewColumn {
            role: "credit"
            title: "Credit"
            width: 120
        }
        TableViewColumn {
            role: "imagesource"
            title: "Image source"
            width: 200
            visible: true
        }
    }
}
