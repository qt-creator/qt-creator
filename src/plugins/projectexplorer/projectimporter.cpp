/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectimporter.h"

#include "kit.h"
#include "kitmanager.h"
#include "project.h"

#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>

namespace ProjectExplorer {

static const Core::Id KIT_IS_TEMPORARY("PE.TempKit");
static const Core::Id KIT_TEMPORARY_NAME("PE.TempName");
static const Core::Id KIT_FINAL_NAME("PE.FinalName");
static const Core::Id TEMPORARY_OF_PROJECTS("PE.TempProject");

ProjectImporter::ProjectImporter(const QString &path) : m_projectPath(path), m_isUpdating(false)
{ }

ProjectImporter::~ProjectImporter()
{
    foreach (Kit *k, KitManager::kits())
        removeProject(k, m_projectPath);
}

void ProjectImporter::markTemporary(Kit *k)
{
    QTC_ASSERT(!k->hasValue(KIT_IS_TEMPORARY), return);

    bool oldIsUpdating = setIsUpdating(true);

    const QString name = k->displayName();
    k->setUnexpandedDisplayName(QCoreApplication::translate("ProjectExplorer::ProjectImporter",
                                                  "%1 - temporary").arg(name));

    k->setValue(KIT_TEMPORARY_NAME, k->displayName());
    k->setValue(KIT_FINAL_NAME, name);
    k->setValue(KIT_IS_TEMPORARY, true);

    setIsUpdating(oldIsUpdating);
}

void ProjectImporter::makePermanent(Kit *k)
{
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    bool oldIsUpdating = setIsUpdating(true);

    k->removeKey(KIT_IS_TEMPORARY);
    k->removeKey(TEMPORARY_OF_PROJECTS);
    const QString tempName = k->value(KIT_TEMPORARY_NAME).toString();
    if (!tempName.isNull() && k->displayName() == tempName)
        k->setUnexpandedDisplayName(k->value(KIT_FINAL_NAME).toString());
    k->removeKey(KIT_TEMPORARY_NAME);
    k->removeKey(KIT_FINAL_NAME);

    setIsUpdating(oldIsUpdating);
}

void ProjectImporter::cleanupKit(Kit *k)
{
    Q_UNUSED(k);
}

void ProjectImporter::addProject(Kit *k)
{
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();

    projects.append(m_projectPath); // note: There can be more than one instance of the project added!

    bool oldIsUpdating = setIsUpdating(true);

    k->setValueSilently(TEMPORARY_OF_PROJECTS, projects);

    setIsUpdating(oldIsUpdating);
}

void ProjectImporter::removeProject(Kit *k, const QString &path)
{
    if (!k->hasValue(KIT_IS_TEMPORARY))
        return;

    QStringList projects = k->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
    projects.removeOne(path);

    bool oldIsUpdating = setIsUpdating(true);

    if (projects.isEmpty())
        KitManager::deregisterKit(k);
    else
        k->setValueSilently(TEMPORARY_OF_PROJECTS, projects);

    setIsUpdating(oldIsUpdating);
}

bool ProjectImporter::isTemporaryKit(Kit *k)
{
    return k->hasValue(KIT_IS_TEMPORARY);
}

} // namespace ProjectExplorer
