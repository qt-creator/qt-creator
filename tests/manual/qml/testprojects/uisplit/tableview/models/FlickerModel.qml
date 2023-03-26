// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2
import QtQuick.XmlListModel 2.0

XmlListModel {
    source: "http://api.flickr.com/services/feeds/photos_public.gne?format=rss2&tags=" + "Qt"
    query: "/rss/channel/item"
    namespaceDeclarations: "declare namespace media=\"http://search.yahoo.com/mrss/\";"
    XmlRole { name: "title"; query: "title/string()" }
    XmlRole { name: "imagesource"; query: "media:thumbnail/@url/string()" }
    XmlRole { name: "credit"; query: "media:credit/string()" }
}
