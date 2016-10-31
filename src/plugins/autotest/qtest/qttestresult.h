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

#pragma once

#include "../testresult.h"

namespace Autotest {
namespace Internal {

class QtTestResult : public TestResult
{
public:
    explicit QtTestResult(const QString &className = QString());
    const QString outputString(bool selected) const override;

    void setFunctionName(const QString &functionName) { m_function = functionName; }
    void setDataTag(const QString &dataTag) { m_dataTag = dataTag; }

    bool isDirectParentOf(const TestResult *other, bool *needsIntermediate) const override;
    bool isIntermediateFor(const TestResult *other) const override;
    TestResult *createIntermediateResultFor(const TestResult *other) override;
private:
    bool isTestCase() const     { return m_function.isEmpty()  && m_dataTag.isEmpty(); }
    bool isTestFunction() const { return !m_function.isEmpty() && m_dataTag.isEmpty(); }
    bool isDataTag() const      { return !m_function.isEmpty() && !m_dataTag.isEmpty(); }

    QString m_function;
    QString m_dataTag;
};

} // namespace Internal
} // namespace Autotest
