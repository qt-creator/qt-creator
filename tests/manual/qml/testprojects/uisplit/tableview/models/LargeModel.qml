// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.1
import QtQuick.Controls 1.2

ListModel {
    id: largeModel
    Component.onCompleted: {
        for (var i=0 ; i< 500 ; ++i)
            largeModel.append({"name":"Person "+i , "age": Math.round(Math.random()*100), "gender": Math.random()>0.5 ? "Male" : "Female"})
    }
}
