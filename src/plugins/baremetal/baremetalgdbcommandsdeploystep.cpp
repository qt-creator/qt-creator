/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetalgdbcommandsdeploystep.h"

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

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
    connect(m_commands, SIGNAL(textChanged()), SLOT(update()));
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

} // namespace Internal

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

BareMetalGdbCommandsDeployStep::~BareMetalGdbCommandsDeployStep()
{
}

void BareMetalGdbCommandsDeployStep::run(QFutureInterface<bool> &fi)
{
    fi.reportResult(true);
    emit finished();
}

bool BareMetalGdbCommandsDeployStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    m_gdbCommands = map.value(QLatin1String(Internal::GdbCommandsKey)).toString();
    return true;
}

QVariantMap BareMetalGdbCommandsDeployStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
    map.insert(QLatin1String(Internal::GdbCommandsKey),m_gdbCommands);
    return map;
}

ProjectExplorer::BuildStepConfigWidget *BareMetalGdbCommandsDeployStep::createConfigWidget()
{
    return new Internal::BareMetalGdbCommandsDeployStepWidget(*this);
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

bool BareMetalGdbCommandsDeployStep::init()
{
    return true;
}

} // namespace BareMetal
