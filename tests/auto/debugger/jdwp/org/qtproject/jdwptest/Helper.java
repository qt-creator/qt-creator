// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.jdwptest;

// A second top-level class in a file named after another one, whose static
// goes by its plain name from the class below it.
class Shared
{
    static int counted = 17;
    int level = 1;
}

// Loaded only once main() gets to it, and a class of its own besides.
class Helper
{
    static class Nested extends Shared
    {
        private final int value;
        // Hides the one of the superclass.
        int level = 2;

        Nested(int value)
        {
            this.value = value;
        }

        int describe()
        {
            int doubled = value * 2; // nested-body
            return doubled;
        }

        // A local of the same name as the field, which is what an expression
        // has to prefer.
        int shadow()
        {
            int value = 42;
            return value; // nested-shadow
        }
    }
}
