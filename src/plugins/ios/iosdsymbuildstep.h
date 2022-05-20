/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
