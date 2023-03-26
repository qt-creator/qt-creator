// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environment.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>

#define ASSERT_STATE_GENERIC(State, expected, actual)                         \
    AndroidGlobal::assertState<State>(expected, actual, Q_FUNC_INFO)

namespace Android {

class AndroidGlobal
{
public:
    template<typename State> static void assertState(State expected,
        State actual, const char *func)
    {
        assertState(QList<State>() << expected, actual, func);
    }

    template<typename State> static void assertState(const QList<State> &expected,
        State actual, const char *func)
    {
        if (!expected.contains(actual)) {
            qWarning("Warning: Unexpected state %d in function %s.",
                actual, func);
        }
    }
};

} // namespace Android
