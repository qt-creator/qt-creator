// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>

namespace Ios {
namespace Internal {

class IosDsymBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    IosDsymBuildStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    QWidget *createConfigWidget() override;
    void setArguments(const QStringList &args);
    QStringList arguments() const;
    QStringList defaultArguments() const;
    Utils::FilePath defaultCommand() const;
    Utils::FilePath command() const;
    void setCommand(const Utils::FilePath &command);
    bool isDefault() const;

private:
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

    QStringList defaultCleanCmdList() const;
    QStringList defaultCmdList() const;

    QStringList m_arguments;
    Utils::FilePath m_command;
    bool m_clean;
};

class IosDsymBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    explicit IosDsymBuildStepFactory();
};

} // namespace Internal
} // namespace Ios
