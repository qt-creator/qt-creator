// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.4

Item {
    anchors.fill: parent
    MyListView {
    /*
       The contact model might be provided by C++ code,
       but the we provide a mockup model in dummydata.
    */
        model: contactModel
    }
}
