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

#include "remotelinuxcheckforfreediskspacestep.h"
#include "ui_remotelinuxcheckforfreediskspacestepwidget.h"

#include "remotelinuxcheckforfreediskspaceservice.h"

#include <QString>

#include <limits>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
class RemoteLinuxCheckForFreeDiskSpaceStepWidget : public BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit RemoteLinuxCheckForFreeDiskSpaceStepWidget(RemoteLinuxCheckForFreeDiskSpaceStep &step)
            : m_step(step)
    {
        m_ui.setupUi(this);
        m_ui.requiredSpaceSpinBox->setSuffix(tr("MB"));
        m_ui.requiredSpaceSpinBox->setMinimum(1);
        m_ui.requiredSpaceSpinBox->setMaximum(std::numeric_limits<int>::max());

        m_ui.pathLineEdit->setText(m_step.pathToCheck());
        m_ui.requiredSpaceSpinBox->setValue(m_step.requiredSpaceInBytes()/multiplier);

        connect(m_ui.pathLineEdit, &QLineEdit::textChanged,
                this, &RemoteLinuxCheckForFreeDiskSpaceStepWidget::handlePathChanged);
        connect(m_ui.requiredSpaceSpinBox,
                static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                this, &RemoteLinuxCheckForFreeDiskSpaceStepWidget::handleRequiredSpaceChanged);
    }

private:
    void handlePathChanged() { m_step.setPathToCheck(m_ui.pathLineEdit->text().trimmed()); }
    void handleRequiredSpaceChanged() {
        m_step.setRequiredSpaceInBytes(quint64(m_ui.requiredSpaceSpinBox->value())*multiplier);
    }

    QString displayName() const { return QLatin1String("<b>") + m_step.displayName()
            + QLatin1String("</b>"); }
    QString summaryText() const { return displayName(); }

    static const int multiplier = 1024*1024;

    Ui::RemoteLinuxCheckForFreeDiskSpaceStepWidget m_ui;
    RemoteLinuxCheckForFreeDiskSpaceStep &m_step;
};

} // anonymous namespace

const char PathToCheckKey[] = "RemoteLinux.CheckForFreeDiskSpaceStep.PathToCheck";
const char RequiredSpaceKey[] = "RemoteLinux.CheckForFreeDiskSpaceStep.RequiredSpace";

class RemoteLinuxCheckForFreeDiskSpaceStepPrivate
{
public:
    RemoteLinuxCheckForFreeDiskSpaceService deployService;
    QString pathToCheck;
    quint64 requiredSpaceInBytes;
};
} // namespace Internal


RemoteLinuxCheckForFreeDiskSpaceStep::RemoteLinuxCheckForFreeDiskSpaceStep(BuildStepList *bsl, Core::Id id)
        : AbstractRemoteLinuxDeployStep(bsl, id)
{
    ctor();
    setPathToCheck(QLatin1String("/"));
    setRequiredSpaceInBytes(5*1024*1024);
}

RemoteLinuxCheckForFreeDiskSpaceStep::RemoteLinuxCheckForFreeDiskSpaceStep(BuildStepList *bsl,
        RemoteLinuxCheckForFreeDiskSpaceStep *other) : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
    setPathToCheck(other->pathToCheck());
    setRequiredSpaceInBytes(other->requiredSpaceInBytes());
}

RemoteLinuxCheckForFreeDiskSpaceStep::~RemoteLinuxCheckForFreeDiskSpaceStep()
{
    delete d;
}

void RemoteLinuxCheckForFreeDiskSpaceStep::ctor()
{
    d = new Internal::RemoteLinuxCheckForFreeDiskSpaceStepPrivate;
    setDefaultDisplayName(stepDisplayName());
}

void RemoteLinuxCheckForFreeDiskSpaceStep::setPathToCheck(const QString &path)
{
    d->pathToCheck = path;
}

QString RemoteLinuxCheckForFreeDiskSpaceStep::pathToCheck() const
{
    return d->pathToCheck;
}

void RemoteLinuxCheckForFreeDiskSpaceStep::setRequiredSpaceInBytes(quint64 space)
{
    d->requiredSpaceInBytes = space;
}

quint64 RemoteLinuxCheckForFreeDiskSpaceStep::requiredSpaceInBytes() const
{
    return d->requiredSpaceInBytes;
}

bool RemoteLinuxCheckForFreeDiskSpaceStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    d->pathToCheck = map.value(QLatin1String(Internal::PathToCheckKey)).toString();
    d->requiredSpaceInBytes = map.value(QLatin1String(Internal::RequiredSpaceKey)).toULongLong();
    return true;
}

QVariantMap RemoteLinuxCheckForFreeDiskSpaceStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(QLatin1String(Internal::PathToCheckKey), d->pathToCheck);
    map.insert(QLatin1String(Internal::RequiredSpaceKey), d->requiredSpaceInBytes);
    return map;
}

BuildStepConfigWidget *RemoteLinuxCheckForFreeDiskSpaceStep::createConfigWidget()
{
    return new Internal::RemoteLinuxCheckForFreeDiskSpaceStepWidget(*this);
}

bool RemoteLinuxCheckForFreeDiskSpaceStep::initInternal(QString *error)
{
    Q_UNUSED(error);
    d->deployService.setPathToCheck(d->pathToCheck);
    d->deployService.setRequiredSpaceInBytes(d->requiredSpaceInBytes);
    return true;
}

AbstractRemoteLinuxDeployService *RemoteLinuxCheckForFreeDiskSpaceStep::deployService() const
{
    return &d->deployService;
}

Core::Id RemoteLinuxCheckForFreeDiskSpaceStep::stepId()
{
    return "RemoteLinux.CheckForFreeDiskSpaceStep";
}

QString RemoteLinuxCheckForFreeDiskSpaceStep::stepDisplayName()
{
    return tr("Check for free disk space");
}

} // namespace RemoteLinux

#include "remotelinuxcheckforfreediskspacestep.moc"
