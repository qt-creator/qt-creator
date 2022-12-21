// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include <projectexplorer/ioutputparser.h>

#include <QRegularExpression>

namespace QmakeProjectManager {

class QMAKEPROJECTMANAGER_EXPORT QMakeParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    QMakeParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    const QRegularExpression m_error;
};

} // namespace QmakeProjectManager
