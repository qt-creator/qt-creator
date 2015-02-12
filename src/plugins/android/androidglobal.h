/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDGLOBAL_H
#define ANDROIDGLOBAL_H

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
            return 0;
        foreach (const Core::Id &id, dc->knownStepLists()) {
            ProjectExplorer::BuildStepList *bsl = dc->stepList(id);
            if (!bsl)
                return 0;
            const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
            for (int i = buildSteps.count() - 1; i >= 0; --i) {
                if (T * const step = qobject_cast<T *>(buildSteps.at(i)))
                    return step;
            }
        }
        return 0;
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

#endif // ANDROIDGLOBAL_H
