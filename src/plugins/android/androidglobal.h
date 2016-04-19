/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    template<class T> static T *buildStep(const ProjectExplorer::BuildConfiguration *dc)
    {
        if (!dc)
            return nullptr;
        foreach (const Core::Id &id, dc->knownStepLists()) {
            T *const step = dc->stepList(id)->firstOfType<T>();
            if (step)
                return step;
        }
        return nullptr;
    }

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
