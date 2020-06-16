/****************************************************************************
**
** Copyright (C) 2016 Axonian LLC.
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

#include "cmake_global.h"

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <utils/optional.h>

#include <QDir>
#include <QRegularExpression>

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    explicit CMakeParser();
    void setSourceDirectory(const QString &sourceDir);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    enum TripleLineError { NONE, LINE_LOCATION, LINE_DESCRIPTION, LINE_DESCRIPTION2 };

    TripleLineError m_expectTripleLineErrorData = NONE;

    Utils::optional<QDir> m_sourceDirectory;
    ProjectExplorer::Task m_lastTask;
    QRegularExpression m_commonError;
    QRegularExpression m_nextSubError;
    QRegularExpression m_locationLine;
    bool m_skippedFirstEmptyLine = false;
    int m_lines = 0;
};

} // namespace CMakeProjectManager
