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

class PROJECTEXPLORER_EXPORT GnuMakeParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit GnuMakeParser();

    void stdOutput(const QString &line) override;
    void stdError(const QString &line) override;

    void setWorkingDirectory(const QString &workingDirectory) override;

    QStringList searchDirectories() const;

    bool hasFatalErrors() const override;

    void taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skippedLines) override;

private:
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);

    QRegularExpression m_makeDir;
    QRegularExpression m_makeLine;
    QRegularExpression m_threeStarError;
    QRegularExpression m_errorInMakefile;

    QStringList m_directories;

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
    explicit GnuMakeParserTester(GnuMakeParser *parser, QObject *parent = 0);
    void parserIsAboutToBeDeleted();

    QStringList directories;
    GnuMakeParser *parser;
};
#endif

} // namespace ProjectExplorer
