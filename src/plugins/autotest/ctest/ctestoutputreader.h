/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "../testoutputreader.h"

#include <QCoreApplication>

namespace Autotest {
namespace Internal {

class CTestOutputReader final : public Autotest::TestOutputReader
{
    Q_DECLARE_TR_FUNCTIONS(Autotest::Internal::CTestOutputReader)
public:
    CTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                      QProcess *testApplication, const Utils::FilePath &buildDirectory);

protected:
    void processOutputLine(const QByteArray &outputLineWithNewLine) final;
    TestResultPtr createDefaultResult() const final;
    void sendCompleteInformation();
    QString m_project;
    QString m_testName;
    QString m_description;
    ResultType m_result = ResultType::Invalid;
    bool m_expectExceptionFromCrash = false;
};

} // namespace Internal
} // namespace Autotest
