// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
