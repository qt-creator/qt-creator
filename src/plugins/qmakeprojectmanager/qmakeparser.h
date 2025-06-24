// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/ioutputparser.h>

#include <QRegularExpression>

namespace QmakeProjectManager {

class QMAKEPROJECTMANAGER_EXPORT QMakeParser : public ProjectExplorer::OutputTaskParser
{
public:
    QMakeParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    const QRegularExpression m_error;
};

namespace Internal {

class QmakeTask : public ProjectExplorer::BuildSystemTask
{
public:
    QmakeTask(TaskType type, const QString &description, const Utils::FilePath &file = {},
              int line = -1)
        : ProjectExplorer::BuildSystemTask(type, description, file, line)
    {
        setOrigin("qmake");
    }
};

#ifdef WITH_TESTS
QObject *createQmakeOutputParserTest();
#endif

} // namespace Internal
} // namespace QmakeProjectManager
