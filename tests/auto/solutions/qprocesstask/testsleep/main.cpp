// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <thread>

using namespace std;
using namespace std::chrono_literals;

int main()
{
    this_thread::sleep_for(10s);
    return 0;
}
