// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "ioutputparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT XcodebuildParser : public OutputTaskParser
{
    Q_OBJECT
public:
    enum XcodebuildStatus {
        InXcodebuild,
        OutsideXcodebuild,
        UnknownXcodebuildState
    };

    XcodebuildParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    bool hasDetectedRedirection() const override;
    bool hasFatalErrors() const override { return m_fatalErrorCount > 0; }

    int m_fatalErrorCount = 0;
    const QRegularExpression m_failureRe;
    const QRegularExpression m_successRe;
    const QRegularExpression m_buildRe;
    XcodebuildStatus m_xcodeBuildParserState = OutsideXcodebuild;

#if defined WITH_TESTS
    friend class XcodebuildParserTester;
    friend class ProjectExplorerPlugin;
#endif
};

#if defined WITH_TESTS
class XcodebuildParserTester : public QObject
{
    Q_OBJECT
public:
    explicit XcodebuildParserTester(XcodebuildParser *parser, QObject *parent = nullptr);

    XcodebuildParser *parser;
    XcodebuildParser::XcodebuildStatus expectedFinalState = XcodebuildParser::OutsideXcodebuild;

public slots:
    void onAboutToDeleteParser();
};
#endif

} // namespace ProjectExplorer
