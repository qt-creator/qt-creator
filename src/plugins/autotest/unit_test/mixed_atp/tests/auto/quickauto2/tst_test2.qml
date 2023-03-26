// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0
import QtTest 1.0

Rectangle {
    TestCase {
        name: "nestedTC"

        function test_str() {
            var bla = String();
            bla = bla.concat("Hello", " ", "World");
            var blubb = String("Hello World");
            compare(blubb, bla, "Comparing concat");
            verify(blubb == bla, "Comparing concat equality")
        }
    }
}
