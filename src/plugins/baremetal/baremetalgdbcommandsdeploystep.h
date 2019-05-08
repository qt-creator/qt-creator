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

#include <QPlainTextEdit>
#include <QVariantMap>

namespace BareMetal {
namespace Internal {

// BareMetalGdbCommandsDeployStep

class BareMetalGdbCommandsDeployStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit BareMetalGdbCommandsDeployStep(ProjectExplorer::BuildStepList *bsl);

    bool fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() final;

    static Core::Id stepId();
    static QString displayName();

    void updateGdbCommands(const QString &newCommands);
    QString gdbCommands() const;

private:
    bool init() final;
    void doRun() final;

    QString m_gdbCommands;
};

// BareMetalGdbCommandsDeployStepWidget

class BareMetalGdbCommandsDeployStepWidget final
        : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit BareMetalGdbCommandsDeployStepWidget(BareMetalGdbCommandsDeployStep &step);
    void update();

private:
    QString displayName() const;
    QString summaryText() const;

    BareMetalGdbCommandsDeployStep &m_step;
    QPlainTextEdit *m_commands = nullptr;
};

} // namespace Internal
} // namespace BareMetal
