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

#include "ioutputparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GnuMakeParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    explicit GnuMakeParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    bool hasFatalErrors() const override;

    void emitTask(const ProjectExplorer::Task &task);

    QRegularExpression m_makeDir;
    QRegularExpression m_makeLine;
    QRegularExpression m_threeStarError;
    QRegularExpression m_errorInMakefile;

    bool m_suppressIssues = false;

    int m_fatalErrorCount = 0;

#if defined WITH_TESTS
    friend class ProjectExplorerPlugin;
#endif
};

#if defined WITH_TESTS
class GnuMakeParserTester : public QObject
{
    Q_OBJECT

public:
    explicit GnuMakeParserTester(GnuMakeParser *parser, QObject *parent = nullptr);
    void parserIsAboutToBeDeleted();

    Utils::FilePaths directories;
    GnuMakeParser *parser;
};
#endif

} // namespace ProjectExplorer
