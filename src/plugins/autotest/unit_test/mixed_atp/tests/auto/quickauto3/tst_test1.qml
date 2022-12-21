// Copyright (C) 2020 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0

Bar {
    name: "InheritanceTest"

    function test_func() {
        compare(5 + 5, 10, "verifying 5 + 5 = 10");
        compare(10 - 5, 5, "verifying 10 - 5 = 5");
    }
}
