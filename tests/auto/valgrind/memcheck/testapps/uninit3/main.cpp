// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

static int readThroughPointer(bool initialize)
{
    int *i;
    if (initialize)
        i = nullptr;
    else
        return *i;
    return 0;
}

int main()
{
    // The first call leaves a null pointer in the stack slot of the second.
    readThroughPointer(true);
    return readThroughPointer(false);
}
