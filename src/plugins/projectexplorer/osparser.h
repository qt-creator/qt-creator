// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT OsParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    OsParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    bool hasFatalErrors() const override { return m_hasFatalError; }

    bool m_hasFatalError = false;
};

} // namespace ProjectExplorer
