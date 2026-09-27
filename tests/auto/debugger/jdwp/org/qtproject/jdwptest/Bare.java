// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.jdwptest;

// Compiled a second time without the table that names local variables, which
// is what a class out of a jar that was built without debug information looks
// like. The lines are kept, so a breakpoint still finds its way in.
public class Bare
{
    static int work(int value)
    {
        int doubled = value + value; // bare-body
        return doubled;
    }
}
