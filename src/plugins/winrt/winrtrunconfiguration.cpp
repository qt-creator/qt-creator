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

#include "winrtrunconfiguration.h"
#include "winrtconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runnables.h>

#include <utils/detailswidget.h>

#include <qmakeprojectmanager/qmakeproject.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace WinRt {
namespace Internal {

class UninstallAfterStopAspect : public BaseBoolAspect
{
    Q_OBJECT
public:
    UninstallAfterStopAspect(RunConfiguration *rc)
        : BaseBoolAspect(rc, "WinRtRunConfigurationUninstallAfterStopId")
    {
        setLabel(WinRtRunConfiguration::tr("Uninstall package after application stops"));
    }
};


WinRtRunConfiguration::WinRtRunConfiguration(Target *target)
    : RunConfiguration(target, Constants::WINRT_RC_PREFIX)
{
    setDisplayName(tr("Run App Package"));
    addExtraAspect(new ArgumentsAspect(this, "WinRtRunConfigurationArgumentsId"));
    addExtraAspect(new UninstallAfterStopAspect(this));
}

QWidget *WinRtRunConfiguration::createConfigurationWidget()
{
    auto widget = new QWidget;
    auto fl = new QFormLayout(widget);

    extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(widget, fl);
    extraAspect<UninstallAfterStopAspect>()->addToMainConfigurationWidget(widget, fl);

    auto wrapped = wrapWidget(widget);
    auto detailsWidget = qobject_cast<DetailsWidget *>(wrapped);
    QTC_ASSERT(detailsWidget, return wrapped);
    detailsWidget->setState(DetailsWidget::Expanded);
    detailsWidget->setSummaryText(tr("Launch App"));
    return detailsWidget;
}

bool WinRtRunConfiguration::uninstallAfterStop() const
{
    return extraAspect<UninstallAfterStopAspect>()->value();
}

QString WinRtRunConfiguration::proFilePath() const
{
    return buildKey();
}

QString WinRtRunConfiguration::arguments() const
{
    return extraAspect<ProjectExplorer::ArgumentsAspect>()->arguments();
}

ProjectExplorer::Runnable WinRtRunConfiguration::runnable() const
{
    ProjectExplorer::StandardRunnable r;
    r.executable = executable();
    r.commandLineArguments = arguments();
    return r;
}

QString WinRtRunConfiguration::executable() const
{
    QmakeProjectManager::QmakeProject *project
            = static_cast<QmakeProjectManager::QmakeProject *>(target()->project());
    if (!project)
        return QString();

    QmakeProjectManager::QmakeProFile *rootProFile = project->rootProFile();
    if (!rootProFile)
        return QString();

    const QmakeProjectManager::QmakeProFile *pro
            = rootProFile->findProFile(Utils::FileName::fromString(proFilePath()));
    if (!pro)
        return QString();

    QmakeProjectManager::TargetInformation ti = pro->targetInformation();
    if (!ti.valid)
        return QString();

    QString destDir = ti.destDir.toString();
    if (destDir.isEmpty())
        destDir = ti.buildDir.toString();
    else if (QDir::isRelativePath(destDir))
        destDir = QDir::cleanPath(ti.buildDir.toString() + '/' + destDir);

    QString executable = QDir::cleanPath(destDir + '/' + ti.target);
    executable = Utils::HostOsInfo::withExecutableSuffix(executable);
    return executable;
}

} // namespace Internal
} // namespace WinRt

#include "winrtrunconfiguration.moc"
