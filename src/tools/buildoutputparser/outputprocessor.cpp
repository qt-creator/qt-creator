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

#include "outputprocessor.h"

#include <projectexplorer/clangparser.h>
#include <projectexplorer/gccparser.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/msvcparser.h>
#include <projectexplorer/osparser.h>
#include <projectexplorer/taskhub.h>
#include <qmakeprojectmanager/qmakeparser.h>
#include <qtsupport/qtparser.h>
#include <utils/fileutils.h>
#include <utils/outputformatter.h>

#include <QIODevice>
#include <QTextStream>

#include <stdio.h>

CompilerOutputProcessor::CompilerOutputProcessor(CompilerType compilerType, QIODevice &source)
    : m_compilerType(compilerType)
    , m_source(source)
    , m_ostream(new QTextStream(stdout, QIODevice::WriteOnly))
{
}

CompilerOutputProcessor::~CompilerOutputProcessor()
{
    delete m_ostream;
}

void CompilerOutputProcessor::start()
{
    Utils::OutputFormatter parser;
    parser.addLineParser(new ProjectExplorer::OsParser);
    parser.addLineParser(new QmakeProjectManager::QMakeParser);
    parser.addLineParser(new ProjectExplorer::GnuMakeParser);
    parser.addLineParser(new QtSupport::QtParser);
    switch (m_compilerType) {
    case CompilerTypeGcc:
        parser.addLineParsers(ProjectExplorer::GccParser::gccParserSuite());
        break;
    case CompilerTypeClang:
        parser.addLineParsers(ProjectExplorer::ClangParser::clangParserSuite());
        break;
    case CompilerTypeMsvc:
        parser.addLineParser(new ProjectExplorer::MsvcParser);
        break;
    }

    connect(ProjectExplorer::TaskHub::instance(), &ProjectExplorer::TaskHub::taskAdded,
            this, &CompilerOutputProcessor::handleTask);
    while (!m_source.atEnd()) {
        parser.appendMessage(QString::fromLocal8Bit(m_source.readLine().trimmed()),
                             Utils::StdErrFormat);
    }
    QCoreApplication::quit();
}

void CompilerOutputProcessor::handleTask(const ProjectExplorer::Task &task)
{
    const QString &fileName = task.file.toString();
    if (!fileName.isEmpty()) {
        *m_ostream << fileName;
        if (task.line != -1)
            *m_ostream << ':' << task.line;
        *m_ostream << ": ";
    }
    *m_ostream << task.description() << '\n';
}
