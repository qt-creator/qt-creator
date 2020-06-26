/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace BareMetal {
namespace Internal {

// KeilParser

class KeilParser final : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    explicit KeilParser();
    static Utils::Id id();

private:
    void newTask(const ProjectExplorer::Task &task);

    // ARM compiler specific parsers.
    Result parseArmWarningOrErrorDetailsMessage(const QString &lne);
    bool parseArmErrorOrFatalErorrMessage(const QString &lne);

    // MCS51 compiler specific parsers.
    Result parseMcs51WarningOrErrorDetailsMessage1(const QString &lne);
    Result parseMcs51WarningOrErrorDetailsMessage2(const QString &lne);
    bool parseMcs51WarningOrFatalErrorMessage(const QString &lne);
    bool parseMcs51FatalErrorMessage2(const QString &lne);

    Result handleLine(const QString &line, Utils::OutputFormat type) final;
    void flush() final;

    ProjectExplorer::Task m_lastTask;
    int m_lines = 0;
    QStringList m_snippets;
};

} // namespace Internal
} // namespace BareMetal
