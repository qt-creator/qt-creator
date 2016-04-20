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

#include "baremetalgdbcommandsdeploystep.h"

#include <QFormLayout>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

const char GdbCommandsKey[] = "BareMetal.GdbCommandsStep.Commands";

BareMetalGdbCommandsDeployStepWidget::BareMetalGdbCommandsDeployStepWidget(BareMetalGdbCommandsDeployStep &step)
    : m_step(step)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(fl);
    m_commands = new QPlainTextEdit(this);
    fl->addRow(tr("GDB commands:"), m_commands);
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

BareMetalGdbCommandsDeployStep::BareMetalGdbCommandsDeployStep(BuildStepList *bsl,
                                                               const Core::Id id)
    : BuildStep(bsl, id)
{
    ctor();
}

BareMetalGdbCommandsDeployStep::BareMetalGdbCommandsDeployStep(BuildStepList *bsl,
                                            BareMetalGdbCommandsDeployStep *other)
    : BuildStep(bsl, other)
{
    ctor();
}

void BareMetalGdbCommandsDeployStep::ctor()
{
    setDefaultDisplayName(displayName());
}

void BareMetalGdbCommandsDeployStep::run(QFutureInterface<bool> &fi)
{
    reportRunResult(fi, true);
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
    return tr("GDB commands");
}

void BareMetalGdbCommandsDeployStep::updateGdbCommands(const QString &newCommands)
{
    m_gdbCommands = newCommands;
}

QString BareMetalGdbCommandsDeployStep::gdbCommands() const
{
    return m_gdbCommands;
}

bool BareMetalGdbCommandsDeployStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    return true;
}

} // namespace Internal
} // namespace BareMetal
