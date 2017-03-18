/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include <projectexplorer/buildstep.h>

#include <QVariantMap>
#include <QPlainTextEdit>

namespace BareMetal {
namespace Internal {

class BareMetalGdbCommandsDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    BareMetalGdbCommandsDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    BareMetalGdbCommandsDeployStep(ProjectExplorer::BuildStepList *bsl,
                                   BareMetalGdbCommandsDeployStep *other);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    bool runInGuiThread() const override { return true;}

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    static Core::Id stepId();
    static QString displayName();

    void updateGdbCommands(const QString &newCommands);
    QString gdbCommands() const;

private:
    void ctor();
    QString m_gdbCommands;
};

class BareMetalGdbCommandsDeployStepWidget: public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit BareMetalGdbCommandsDeployStepWidget(BareMetalGdbCommandsDeployStep &step);
    void update();

private:
    QString displayName() const;
    QString summaryText() const;

    BareMetalGdbCommandsDeployStep &m_step;
    QPlainTextEdit *m_commands;
};

} // namespace Internal
} // namespace BareMetal
