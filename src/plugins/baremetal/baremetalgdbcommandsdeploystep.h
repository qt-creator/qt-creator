// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
