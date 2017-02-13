/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qttestresult.h"

#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

QtTestResult::QtTestResult(const QString &className)
    : TestResult(className)
{
}

const QString QtTestResult::outputString(bool selected) const
{
    const QString &desc = description();
    const QString &className = name();
    QString output;
    switch (result()) {
    case Result::Pass:
    case Result::Fail:
    case Result::ExpectedFail:
    case Result::UnexpectedPass:
    case Result::BlacklistedFail:
    case Result::BlacklistedPass:
        output = className + "::" + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString(" (%1)").arg(m_dataTag));
        if (selected && !desc.isEmpty()) {
            output.append('\n').append(desc);
        }
        break;
    case Result::Benchmark:
        output = className + "::" + m_function;
        if (!m_dataTag.isEmpty())
            output.append(QString(" (%1)").arg(m_dataTag));
        if (!desc.isEmpty()) {
            int breakPos = desc.indexOf('(');
            output.append(": ").append(desc.left(breakPos));
            if (selected)
                output.append('\n').append(desc.mid(breakPos));
        }
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split('\n').first();
    }
    return output;
}

bool QtTestResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);

    if (TestResult::isMessageCaseStart(result())) {
        if (qtOther->isDataTag()) {
            if (qtOther->m_function == m_function) {
                if (m_dataTag.isEmpty()) {
                    // avoid adding function's TestCaseEnd to the data tag
                    *needsIntermediate = qtOther->result() != Result::MessageTestCaseEnd;
                    return true;
                }
                return qtOther->m_dataTag == m_dataTag;
            }
        } else if (qtOther->isTestFunction()) {
            return isTestCase() || m_function == qtOther->m_function;
        }
    }
    return false;
}

bool QtTestResult::isIntermediateFor(const TestResult *other) const
{
    QTC_ASSERT(other, return false);
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);
    return m_dataTag == qtOther->m_dataTag && m_function == qtOther->m_function
            && name() == qtOther->name();
}

TestResult *QtTestResult::createIntermediateResultFor(const TestResult *other)
{
    QTC_ASSERT(other, return nullptr);
    const QtTestResult *qtOther = static_cast<const QtTestResult *>(other);
    QtTestResult *intermediate = new QtTestResult(qtOther->name());
    intermediate->m_function = qtOther->m_function;
    intermediate->m_dataTag = qtOther->m_dataTag;
    // intermediates will be needed only for data tags
    intermediate->setDescription("Data tag: " + qtOther->m_dataTag);
    return intermediate;
}

} // namespace Internal
} // namespace Autotest
