/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "vcprojectkitinformation.h"
#include "vcprojectmanagerconstants.h"
#include "msbuildversionmanager.h"
#include "widgets/vcprojectkitconfigwidget.h"

#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorerconstants.h>

namespace VcProjectManager {
namespace Internal {

VcProjectKitInformation::VcProjectKitInformation()
{
    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();

    connect(msBVM, SIGNAL(msBuildRemoved(Core::Id)), this, SLOT(onMSBuildRemoved(Core::Id)));
    connect(msBVM, SIGNAL(msBuildReplaced(Core::Id, Core::Id)), this, SLOT(onMSBuildReplaced(Core::Id, Core::Id)));
}

VcProjectKitInformation::~VcProjectKitInformation()
{
}

Core::Id VcProjectKitInformation::dataId() const
{
    static Core::Id id = Core::Id(Constants::VC_PROJECT_KIT_INFO_ID);
    return id;
}

unsigned int VcProjectKitInformation::priority() const
{
    return 100;
}

QVariant VcProjectKitInformation::defaultValue(Kit *) const
{
    QList<MsBuildInformation *> msBuilds = MsBuildVersionManager::instance()->msBuildInformations();

    if (msBuilds.size() == 0)
        return QVariant();

    return msBuilds.at(0)->getId().toSetting();
}

QList<Task> VcProjectKitInformation::validate(const Kit *k) const
{
    QList<Task> result;

    const MsBuildInformation* msBuild = msBuildInfo(k);
    if (!msBuild) {
        result << Task(Task::Error, tr("No MS Build in kit."),
                       Utils::FileName(), -1, Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    } else {
        result << QList<Task>();
    }
    return result;
}

void VcProjectKitInformation::fix(Kit *k)
{
    QTC_ASSERT(MsBuildVersionManager::instance(), return);
    if (msBuildInfo(k))
        return;

    qWarning("Ms Build is no longer known, removing from kit \"%s\".",
             qPrintable(k->displayName()));
    setMsBuild(k, 0); // make sure to clear out no longer known Ms Builds
}

void VcProjectKitInformation::setup(Kit *k)
{
    Core::Id id = Core::Id::fromSetting(k->value(Core::Id(Constants::VC_PROJECT_KIT_INFO_ID)));

    if (!id.isValid())
        return;

    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
    MsBuildInformation *msBuild = msBVM->msBuildInformation(id);

    if (msBuild)
        return;

    QList<MsBuildInformation *> msBuilds = msBVM->msBuildInformations();

    if (msBuilds.size() > 0)
        setMsBuild(k, msBuilds.at(0));
}

ProjectExplorer::KitInformation::ItemList VcProjectKitInformation::toUserOutput(const Kit *) const
{
    return ItemList() << qMakePair(tr("MsBuild Version"), tr("None"));
}

ProjectExplorer::KitConfigWidget *VcProjectKitInformation::createConfigWidget(Kit *k) const
{
    return new VcProjectKitConfigWidget(k);
}

MsBuildInformation *VcProjectKitInformation::msBuildInfo(const Kit *k)
{
    if (!k)
        return 0;

    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();
    return msBVM->msBuildInformation(Core::Id::fromSetting(k->value(Core::Id(Constants::VC_PROJECT_KIT_INFO_ID))));
}

void VcProjectKitInformation::setMsBuild(Kit *k, MsBuildInformation *msBuild)
{
    k->setValue(Core::Id(Constants::VC_PROJECT_KIT_INFO_ID), msBuild ? msBuild->getId().toSetting() : QVariant());
}

void VcProjectKitInformation::onMSBuildAdded(Core::Id msBuildId)
{
    Q_UNUSED(msBuildId);
    foreach (Kit *k, KitManager::instance()->kits()) {
        fix(k);
        notifyAboutUpdate(k);
    }
}

void VcProjectKitInformation::onMSBuildRemoved(Core::Id msBuildId)
{
    Q_UNUSED(msBuildId);
    foreach (Kit *k, KitManager::instance()->kits())
        fix(k);
}

void VcProjectKitInformation::onMSBuildReplaced(Core::Id oldMsBuildId, Core::Id newMsBuildId)
{
    Q_UNUSED(oldMsBuildId);
    foreach (Kit *k, KitManager::instance()->kits()) {
        fix(k);
        setMsBuild(k, MsBuildVersionManager::instance()->msBuildInformation(newMsBuildId));
        notifyAboutUpdate(k);
    }
}

} // namespace Internal
} // namespace VcProjectManager
