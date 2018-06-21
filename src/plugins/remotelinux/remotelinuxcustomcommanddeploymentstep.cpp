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

        connect(&m_commandLineEdit, &QLineEdit::textEdited,
                this, &ConfigWidget::handleCommandLineEdited);
    }

    bool showWidget() const { return true; }

private:
    void handleCommandLineEdited() {
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


AbstractRemoteLinuxCustomCommandDeploymentStep::AbstractRemoteLinuxCustomCommandDeploymentStep
        (BuildStepList *bsl, Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    d = new AbstractRemoteLinuxCustomCommandDeploymentStepPrivate;
}

AbstractRemoteLinuxCustomCommandDeploymentStep::~AbstractRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
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
    d = new GenericRemoteLinuxCustomCommandDeploymentStepPrivate;
    setDefaultDisplayName(displayName());
}

GenericRemoteLinuxCustomCommandDeploymentStep::~GenericRemoteLinuxCustomCommandDeploymentStep()
{
    delete d;
}

RemoteLinuxCustomCommandDeployService *GenericRemoteLinuxCustomCommandDeploymentStep::deployService() const
{
    return &d->service;
}

Core::Id GenericRemoteLinuxCustomCommandDeploymentStep::stepId()
{
    return "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
}

QString GenericRemoteLinuxCustomCommandDeploymentStep::displayName()
{
    return tr("Run custom remote command");
}

} // namespace RemoteLinux

#include "remotelinuxcustomcommanddeploymentstep.moc"
