// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
