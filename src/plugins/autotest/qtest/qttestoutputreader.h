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

#include "../testoutputreader.h"

#include <QXmlStreamReader>

namespace Autotest {
namespace Internal {

class QtTestOutputReader : public TestOutputReader
{
public:
    QtTestOutputReader(const QFutureInterface<TestResultPtr> &futureInterface,
                       QProcess *testApplication, const QString &buildDirectory);

protected:
    void processOutput() override;

private:
    enum CDATAMode
    {
        None,
        DataTag,
        Description,
        QtVersion,
        QtBuild,
        QTestVersion
    };

    CDATAMode m_cdataMode = None;
    QString m_className;
    QString m_testCase;
    QString m_dataTag;
    Result::Type m_result = Result::Invalid;
    QString m_description;
    QString m_file;
    int m_lineNumber = 0;
    QString m_duration;
    QXmlStreamReader m_xmlReader;
};

} // namespace Internal
} // namespace Autotest
