// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"
#include "runcontrol.h"
#include "task.h"

#include <QObject>
#include <QVector>

namespace ProjectExplorer::Internal {

class SanitizerParser : public OutputTaskParser
{
public:
    static std::optional<std::function<QObject *()>> testCreator();

private:
    Result handleLine(const QString &line, Utils::OutputFormat format) override;
    void flush() override;

    Result handleContinuation(const QString &line);
    void addLinkSpecs(const LinkSpecs &linkSpecs);

    Task m_task;
    LinkSpecs m_linkSpecs;
    quint64 m_id = 0;
};

class SanitizerOutputFormatterFactory : public ProjectExplorer::OutputFormatterFactory
{
public:
    SanitizerOutputFormatterFactory();
};

} // namespace ProjectExplorer::Internal

