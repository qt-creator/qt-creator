// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

struct LibProbe { int probeValue; };
struct SecondProbe { int secondValue; };

extern "C" __declspec(dllexport) LibProbe *inferiorLibProbe()
{
    static LibProbe probe = {4711};
    return &probe;
}

extern "C" __declspec(dllexport) SecondProbe *inferiorSecondProbe()
{
    static SecondProbe probe = {8822};
    return &probe;
}
