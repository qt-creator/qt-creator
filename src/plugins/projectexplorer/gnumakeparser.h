/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef GNUMAKEPARSER_H
#define GNUMAKEPARSER_H

#include "ioutputparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GnuMakeParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit GnuMakeParser();

    void stdOutput(const QString &line);
    void stdError(const QString &line);

    void setWorkingDirectory(const QString &workingDirectory);

    QStringList searchDirectories() const;

    bool hasFatalErrors() const;

public slots:
    void taskAdded(const ProjectExplorer::Task &task, int linkedLines, int skippedLines);

private:
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);

    QRegularExpression m_makeDir;
    QRegularExpression m_makeLine;
    QRegularExpression m_threeStarError;
    QRegularExpression m_errorInMakefile;

    QStringList m_directories;

#if defined WITH_TESTS
    friend class ProjectExplorerPlugin;
#endif
    bool m_suppressIssues;

    int m_fatalErrorCount;
};

#if defined WITH_TESTS
class GnuMakeParserTester : public QObject
{
    Q_OBJECT

public:
    explicit GnuMakeParserTester(GnuMakeParser *parser, QObject *parent = 0);

    QStringList directories;
    GnuMakeParser *parser;

public slots:
    void parserIsAboutToBeDeleted();
};
#endif

} // namespace ProjectExplorer

#endif // GNUMAKEPARSER_H
