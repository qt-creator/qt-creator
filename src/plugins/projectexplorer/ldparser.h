// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"
#include "task.h"

#include <QRegularExpression>

namespace ProjectExplorer {

class LdParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    LdParser();
private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    QRegularExpression m_ranlib;
    QRegularExpression m_regExpLinker;
    QRegularExpression m_regExpGccNames;

    Task m_incompleteTask;
};

} // namespace ProjectExplorer
