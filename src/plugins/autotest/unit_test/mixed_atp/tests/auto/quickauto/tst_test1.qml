// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0
import QtTest 1.0

TestCase {
    name: "Banana"

    function test_math() {
        compare(5 + 5, 10, "verifying 5 + 5 = 10");
        compare(10 - 5, 5, "verifying 10 - 5 = 5");
    }

    function test_fail() {
        verify(false, "verifying false");
    }
}

