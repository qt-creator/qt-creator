// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2

ListModel {
    id: dummyModel
    Component.onCompleted: {
        for (var i = 0 ; i < 100 ; ++i) {
            append({"index": i, "title": "A title " + i, "imagesource" :"http://someurl.com", "credit" : "N/A"})
        }
    }
}
