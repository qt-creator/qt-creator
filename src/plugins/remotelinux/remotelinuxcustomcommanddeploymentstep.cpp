/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include "remotelinuxcustomcommanddeploymentstep.h"

#include <QString>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {

const char CommandLineKey[] = "RemoteLinuxCustomCommandDeploymentStep.CommandLine";

class ConfigWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    ConfigWidget(AbstractRemoteLinuxCustomCommandDeploymentStep *step)
        : SimpleBuildStepConfigWidget(step)
    {
        QVBoxLayout * const mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        QHBoxLayout * const commandLineLayout = new QHBoxLayout;
        mainLayout->addLayout(commandLineLayout);
        QLabel * const commandLineLabel = new QLabel(tr("Command line:"));
        commandLineLayout->addWidget(commandLineLabel);
        m_commandLineEdit.setText(step->commandLine());
        commandLineLayout->addWidget(&m_commandLineEdit);

        connect(&m_commandLineEdit, SIGNAL(textEdited(QString)), SLOT(handleCommandLineEdited()));
    }

    bool showWidget() const { return true; }

private:
    Q_SLOT void handleCommandLineEdited() {
        AbstractRemoteLinuxCustomCommandDeploymentStep *step =
            qobject_cast<AbstractRemoteLinuxCustomCommandDeploymentStep *>(this->step());
        step->setCommandLine(m_commandLineEdit.text().trimmed());
    }

    QLineEdit m_commandLineEdit;
};

} // anonymous namespace

class AbstractRemoteLinuxCustomCommandDeploymentStepPrivate
{
public:
    QString commandLine;
};

class GenericRemoteLinuxCustomCommandDeploymentStepPrivate
{
public:
    RemoteLinuxCustomCommandDeployService service;
};

} // namespace Internal

using namespace Internal;


AbstractRemoteLinuxCustomCommandDeploymentStep::AbstractRemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl,
        const Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    ctor();
}

AbstractRemoteLinuxCustomCommandDeploymentStep::AbstractRemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl,
        AbstractRemoteLinuxCustomCommandDeploymentStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

AbstractRemoteLinuxCustomCommandDeploymentStep::~AbstractRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

void AbstractRemoteLinuxCustomCommandDeploymentStep::ctor()
{
    d = new AbstractRemoteLinuxCustomCommandDeploymentStepPrivate;
}

bool AbstractRemoteLinuxCustomCommandDeploymentStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    d->commandLine = map.value(QLatin1String(CommandLineKey)).toString();
    return true;
}

QVariantMap AbstractRemoteLinuxCustomCommandDeploymentStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(QLatin1String(CommandLineKey), d->commandLine);
    return map;
}

void AbstractRemoteLinuxCustomCommandDeploymentStep::setCommandLine(const QString &commandLine)
{
    d->commandLine = commandLine;
}

QString AbstractRemoteLinuxCustomCommandDeploymentStep::commandLine() const
{
    return d->commandLine;
}

bool AbstractRemoteLinuxCustomCommandDeploymentStep::initInternal(QString *error)
{
    deployService()->setCommandLine(d->commandLine);
    return deployService()->isDeploymentPossible(error);
}

BuildStepConfigWidget *AbstractRemoteLinuxCustomCommandDeploymentStep::createConfigWidget()
{
    return new ConfigWidget(this);
}


GenericRemoteLinuxCustomCommandDeploymentStep::GenericRemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl)
    : AbstractRemoteLinuxCustomCommandDeploymentStep(bsl, stepId())
{
    ctor();
}

GenericRemoteLinuxCustomCommandDeploymentStep::GenericRemoteLinuxCustomCommandDeploymentStep(BuildStepList *bsl,
        GenericRemoteLinuxCustomCommandDeploymentStep *other)
    : AbstractRemoteLinuxCustomCommandDeploymentStep(bsl, other)
{
    ctor();
}

GenericRemoteLinuxCustomCommandDeploymentStep::~GenericRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

void GenericRemoteLinuxCustomCommandDeploymentStep::ctor()
{
    d = new GenericRemoteLinuxCustomCommandDeploymentStepPrivate;
    setDefaultDisplayName(stepDisplayName());
}

RemoteLinuxCustomCommandDeployService *GenericRemoteLinuxCustomCommandDeploymentStep::deployService() const
{
    return &d->service;
}

Core::Id GenericRemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return Core::Id("RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep");
}

QString GenericRemoteLinuxCustomCommandDeploymentStep::stepDisplayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux

#include "remotelinuxcustomcommanddeploymentstep.moc"
