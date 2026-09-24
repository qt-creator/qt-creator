// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.jdwptest;

// Loaded only once main() gets to it, and a class of its own besides.
class Helper
{
    static class Nested
    {
        private final int value;

        Nested(int value)
        {
            this.value = value;
        }

        int describe()
        {
            int doubled = value * 2; // nested-body
            return doubled;
        }
    }
}
