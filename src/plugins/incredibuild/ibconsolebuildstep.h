/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "commandbuilder.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildsteplist.h>

#include <QList>

namespace IncrediBuild {
namespace Internal {

class IBConsoleBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    explicit IBConsoleBuildStep(ProjectExplorer::BuildStepList *buildStepList, Utils::Id id);
    ~IBConsoleBuildStep() override;

    bool init() override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap    toMap() const override;

    int nice() const { return m_nice; }
    void nice(int nice) { m_nice = nice; }

    bool keepJobNum() const { return m_keepJobNum; }
    void keepJobNum(bool keepJobNum) { m_keepJobNum = keepJobNum; }

    bool forceRemote() const { return m_forceRemote; }
    void forceRemote(bool forceRemote) { m_forceRemote = forceRemote; }

    bool alternate() const { return m_alternate; }
    void alternate(bool alternate) { m_alternate = alternate; }

    const QStringList& supportedCommandBuilders();
    CommandBuilder* commandBuilder() const { return m_activeCommandBuilder; }
    void commandBuilder(const QString &commandBuilder);

    bool loadedFromMap() const { return m_loadedFromMap; }
    void tryToMigrate();

    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;

private:
    void initCommandBuilders();

    int m_nice{0};
    bool m_keepJobNum{false};
    bool m_forceRemote{false};
    bool m_alternate{false};
    ProjectExplorer::BuildStepList *m_earlierSteps{};
    bool m_loadedFromMap{false};
    CommandBuilder* m_activeCommandBuilder{};
    QList<CommandBuilder*> m_commandBuildersList{};
};

} // namespace Internal
} // namespace IncrediBuild
