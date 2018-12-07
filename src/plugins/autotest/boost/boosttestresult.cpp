/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boosttestresult.h"

namespace Autotest {
namespace Internal {

BoostTestResult::BoostTestResult(const QString &id, const QString &projectFile, const QString &name)
    : TestResult(id, name), m_projectFile(projectFile)
{
}

const QString BoostTestResult::outputString(bool selected) const
{
    const QString &desc = description();
    QString output;
    switch (result()) {
    case ResultType::Pass:
    case ResultType::Fail:
        output = m_testCase;
        if (selected && !desc.isEmpty())
            output.append('\n').append(desc);
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split('\n').first();
    }
    return output;
}

bool BoostTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;

    const BoostTestResult *boostOther = static_cast<const BoostTestResult *>(other);

    if (m_testSuite != boostOther->m_testSuite)
        return false;

    if (result() == ResultType::TestStart) {
        if (!boostOther->m_testCase.isEmpty())
            return boostOther->m_testSuite == m_testSuite && boostOther->result() != ResultType::TestStart;

        return boostOther->m_testCase == m_testCase;
    }
    return false;
}

} // namespace Internal
} // namespace Autotest

