/**************************************************************************
**
** Copyright (C) 2013 Kläralvdalens Datakonsult AB, a KDAB Group company.
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
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

#include "cmakelocatorfilter.h"
#include "cmakeproject.h"
#include "makestep.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <utils/fileutils.h>


using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace Utils;

CMakeLocatorFilter::CMakeLocatorFilter()
{
    setId("Build CMake target");
    setDisplayName(tr("Build CMake target"));
    setShortcutString(QLatin1String("cm"));

    ProjectExplorer::SessionManager *sm = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    connect(sm, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(slotProjectListUpdated()));
    connect(sm, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(slotProjectListUpdated()));

    // Initialize the filter
    slotProjectListUpdated();
}

CMakeLocatorFilter::~CMakeLocatorFilter()
{

}

QList<Locator::FilterEntry> CMakeLocatorFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<Locator::FilterEntry> result;

    QList<ProjectExplorer::Project *> projects =
            ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects();
    foreach (ProjectExplorer::Project *p, projects) {
        CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(p);
        if (cmakeProject) {
            foreach (CMakeBuildTarget ct, cmakeProject->buildTargets()) {
                if (ct.title.contains(entry)) {
                    Locator::FilterEntry entry(this, ct.title, cmakeProject->document()->fileName());
                    entry.extraInfo = FileUtils::shortNativePath(
                        FileName::fromString(cmakeProject->document()->fileName()));
                    result.append(entry);
                }
            }
        }
    }

    return result;
}

void CMakeLocatorFilter::accept(Locator::FilterEntry selection) const
{
    // Get the project containing the target selected
    CMakeProject *cmakeProject = 0;

    QList<ProjectExplorer::Project *> projects =
            ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects();
    foreach (ProjectExplorer::Project *p, projects) {
        cmakeProject = qobject_cast<CMakeProject *>(p);
        if (cmakeProject && cmakeProject->document()->fileName() == selection.internalData.toString())
            break;
        cmakeProject = 0;
    }
    if (!cmakeProject)
        return;

    // Find the make step
    MakeStep *makeStep = 0;
    ProjectExplorer::BuildStepList *buildStepList = cmakeProject->activeTarget()->activeBuildConfiguration()
            ->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    for (int i = 0; i < buildStepList->count(); ++i) {
        makeStep = qobject_cast<MakeStep *>(buildStepList->at(i));
        if (makeStep)
            break;
    }
    if (!makeStep)
        return;

    // Change the make step to build only the given target
    QStringList oldTargets = makeStep->buildTargets();
    makeStep->setClean(false);
    makeStep->clearBuildTargets();
    makeStep->setBuildTarget(selection.displayName, true);

    // Build
    ProjectExplorer::ProjectExplorerPlugin::instance()->buildProject(cmakeProject);
    makeStep->setBuildTargets(oldTargets);
}

void CMakeLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

void CMakeLocatorFilter::slotProjectListUpdated()
{
    CMakeProject *cmakeProject = 0;

    QList<ProjectExplorer::Project *> projects =
            ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projects();
    foreach (ProjectExplorer::Project *p, projects) {
        cmakeProject = qobject_cast<CMakeProject *>(p);
        if (cmakeProject)
            break;
    }

    // Enable the filter if there's at least one CMake project
    setEnabled(cmakeProject);
}
