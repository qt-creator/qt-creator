/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "outputparsers/ninjaparser.h"
#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <utils/qtcprocess.h>
#include <QObject>

namespace MesonProjectManager {
namespace Internal {
class NinjaBuildStep final : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    NinjaBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() final;
    Utils::CommandLine command();
    QStringList projectTargets();
    void setBuildTarget(const QString &targetName);
    void setCommandArgs(const QString &args);
    const QString &targetName() const { return m_targetName; }
    Q_SIGNAL void targetListChanged();
    Q_SIGNAL void commandChanged();
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

private:
    void update(bool parsingSuccessful);
    bool init() override;
    void doRun() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QString defaultBuildTarget() const;
    QString m_commandArgs;
    QString m_targetName;
    NinjaParser *m_ninjaParser = nullptr;
};

class MesonBuildStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    MesonBuildStepFactory();
};

} // namespace Internal
} // namespace MesonProjectManager
