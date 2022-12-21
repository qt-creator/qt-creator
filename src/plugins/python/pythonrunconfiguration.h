// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

namespace Python::Internal {

class PySideUicExtraCompiler;

class PythonRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    PythonRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    ~PythonRunConfiguration() override;
    void currentInterpreterChanged();
    QList<PySideUicExtraCompiler *> extraCompilers() const;

private:
    void checkForPySide(const Utils::FilePath &python);
    void updateExtraCompilers();
    Utils::FilePath m_pySideUicPath;

    QList<PySideUicExtraCompiler *> m_extraCompilers;
};

class PythonRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    PythonRunConfigurationFactory();
};

class PythonOutputFormatterFactory : public ProjectExplorer::OutputFormatterFactory
{
public:
    PythonOutputFormatterFactory();
};

} // Python::Internal
