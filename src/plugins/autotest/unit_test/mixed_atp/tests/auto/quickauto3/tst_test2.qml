// Copyright (C) 2020 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0

Rectangle {
    Foo {
        name: "NestedInherited"

        function test_strConcat() {
            var tmp = String();
            tmp = tmp.concat("Hello", " ", "World");
            var dummy = String("Hello World");
            compare(dummy, tmp, "Comparing concat");
            verify(dummy === tmp, "Comparing concat equality")
        }
    }
}
