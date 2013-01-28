/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GNUMAKEPARSER_H
#define GNUMAKEPARSER_H

#include "ioutputparser.h"

#include <QRegExp>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GnuMakeParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit GnuMakeParser();

    virtual void stdOutput(const QString &line);
    virtual void stdError(const QString &line);

    virtual void setWorkingDirectory(const QString &workingDirectory);

    QStringList searchDirectories() const;

    bool hasFatalErrors() const;

public slots:
    virtual void taskAdded(const ProjectExplorer::Task &task);

private:
    void addDirectory(const QString &dir);
    void removeDirectory(const QString &dir);

    QRegExp m_makeDir;
    QRegExp m_makeLine;
    QRegExp m_makefileError;

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
