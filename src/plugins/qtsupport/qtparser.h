// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QRegularExpression>

#include "qtsupport_global.h"
#include <projectexplorer/ioutputparser.h>

namespace QtSupport {

// Parser for Qt-specific utilities like moc, uic, etc.

class QTSUPPORT_EXPORT QtParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    QtParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    QRegularExpression m_mocRegExp;
    QRegularExpression m_uicRegExp;
    QRegularExpression m_translationRegExp;
    const QRegularExpression m_qmlToolsRegExp;
};

} // namespace QtSupport
