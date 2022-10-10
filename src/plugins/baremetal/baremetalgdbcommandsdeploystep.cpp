// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "baremetalgdbcommandsdeploystep.h"

#include <QFormLayout>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

const char GdbCommandsKey[] = "BareMetal.GdbCommandsStep.Commands";

// BareMetalGdbCommandsDeployStepWidget

BareMetalGdbCommandsDeployStepWidget::BareMetalGdbCommandsDeployStepWidget(BareMetalGdbCommandsDeployStep &step)
    : BuildStepConfigWidget(&step), m_step(step)
{
    const auto fl = new QFormLayout(this);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    m_commands = new QPlainTextEdit(this);
    fl->addRow(Tr::tr("GDB commands:"), m_commands);
    m_commands->setPlainText(m_step.gdbCommands());
    connect(m_commands, &QPlainTextEdit::textChanged, this, &BareMetalGdbCommandsDeployStepWidget::update);
}

void BareMetalGdbCommandsDeployStepWidget::update()
{
    m_step.updateGdbCommands(m_commands->toPlainText());
}

QString BareMetalGdbCommandsDeployStepWidget::displayName() const
{
    return QLatin1String("<b>") + m_step.displayName() + QLatin1String("</b>");
}

QString BareMetalGdbCommandsDeployStepWidget::summaryText() const
{
    return displayName();
}

// BareMetalGdbCommandsDeployStep

BareMetalGdbCommandsDeployStep::BareMetalGdbCommandsDeployStep(BuildStepList *bsl)
    : BuildStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

void BareMetalGdbCommandsDeployStep::doRun()
{
    emit finished(true);
}

bool BareMetalGdbCommandsDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    m_gdbCommands = map.value(QLatin1String(GdbCommandsKey)).toString();
    return true;
}

QVariantMap BareMetalGdbCommandsDeployStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(QLatin1String(GdbCommandsKey),m_gdbCommands);
    return map;
}

BuildStepConfigWidget *BareMetalGdbCommandsDeployStep::createConfigWidget()
{
    return new BareMetalGdbCommandsDeployStepWidget(*this);
}

Core::Id BareMetalGdbCommandsDeployStep::stepId()
{
    return Core::Id("BareMetal.GdbCommandsDeployStep");
}

QString BareMetalGdbCommandsDeployStep::displayName()
{
    return Tr::tr("GDB commands");
}

void BareMetalGdbCommandsDeployStep::updateGdbCommands(const QString &newCommands)
{
    m_gdbCommands = newCommands;
}

QString BareMetalGdbCommandsDeployStep::gdbCommands() const
{
    return m_gdbCommands;
}

bool BareMetalGdbCommandsDeployStep::init()
{
    return true;
}

} // namespace Internal
} // namespace BareMetal
