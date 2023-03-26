// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0
import QtTest 1.0

TestCase {
    function test_str() {
        var bla = String();
        bla = bla.concat("Hallo", " ", "Welt");
        var blubb = String("Hallo Welt");
        compare(blubb, bla, "Comparing concat");
        verify(blubb == bla, "Comparing concat equality")
    }

    TestCase {
        name: "boo"

        function test_boo() {
            verify(true);
        }

        TestCase {
            name: "far"

            function test_far() {
                verify(true);
            }
        }

        function test_boo2() { // should not get added to "far", but to "boo"
            verify(false);
        }
    }

    TestCase {
        name: "secondBoo"

        function test_bar() {
            compare(1, 1);
        }
    }

    TestCase { // unnamed
        function test_func() {
            verify(true);
        }
    }

    TestCase { // 2nd unnamed with same function name - legal as long it's a different TestCase
        function test_func() {
            verify(true);
        }
    }
}

