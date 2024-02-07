// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "ioutputparser.h"

#include <QRegularExpression>
#include <QStringList>

namespace ProjectExplorer {

namespace Internal {
class XcodebuildParserTester;
class ProjectExplorerTest;
} // Internal

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
    friend class Internal::XcodebuildParserTester;
    friend class Internal::ProjectExplorerTest;
#endif
};

} // namespace ProjectExplorer
