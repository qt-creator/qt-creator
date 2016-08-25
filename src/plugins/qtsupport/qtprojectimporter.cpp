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

#include "qtprojectimporter.h"

#include "qtkitinformation.h"
#include "qtversionfactory.h"
#include "qtversionmanager.h"

#include <coreplugin/id.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QList>

using namespace ProjectExplorer;

namespace QtSupport {

const Core::Id QT_IS_TEMPORARY("Qt.TempQt");

static void cleanTemporaryVersion(BaseQtVersion *version) {
    if (!version)
        return;

    // count how many kits are using this version
    const int qtId = version->uniqueId();
    const int users = Utils::count(KitManager::kits(), [qtId](Kit *k) {
        return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
    });

    if (users == 0) // Remove if no other kit is using it. (The Kit k is not in KitManager::kits()
        QtVersionManager::removeVersion(version);
}

void QtProjectImporter::makePermanent(Kit *k) const
{
    if (!isTemporaryKit(k))
        return;

    UpdateGuard guard(*this);
    const int tempId = k->value(QT_IS_TEMPORARY, -1).toInt();
    k->removeKeySilently(QT_IS_TEMPORARY);
    const int qtId = QtKitInformation::qtVersionId(k);
    if (tempId != qtId)
        cleanTemporaryVersion(QtVersionManager::version(tempId));

    foreach (Kit *kit, KitManager::kits())
        if (kit->value(QT_IS_TEMPORARY, -1).toInt() == tempId)
            kit->removeKeySilently(QT_IS_TEMPORARY);
    ProjectImporter::makePermanent(k);
}

QtProjectImporter::QtVersionData
QtProjectImporter::findOrCreateQtVersion(const Utils::FileName &qmakePath) const
{
    QtVersionData result;
    result.qt
            = Utils::findOrDefault(QtVersionManager::unsortedVersions(),
                                  [&qmakePath](BaseQtVersion *v) -> bool {
                                      QFileInfo vfi = v->qmakeCommand().toFileInfo();
                                      Utils::FileName current = Utils::FileName::fromString(vfi.canonicalFilePath());
                                      return current == qmakePath;
                                  });

    if (result.qt) {
        // Check if version is a temporary qt
        const int qtId = result.qt->uniqueId();
        result.isTemporary = Utils::anyOf(KitManager::kits(), [&qtId](Kit *k) {
                return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
            });
        return result;
    }

    // Create a new version if not found:
    // Do not use the canonical path here...
    result.qt = QtVersionFactory::createQtVersionFromQMakePath(qmakePath);
    result.isTemporary = true;
    if (result.qt) {
        UpdateGuard guard(*this);
        QtVersionManager::addVersion(result.qt);
    }

    return result;
}

Kit *QtProjectImporter::createTemporaryKit(const QtVersionData &versionData,
                                           const ProjectImporter::KitSetupFunction &setup) const
{
    return ProjectImporter::createTemporaryKit([&setup, &versionData](Kit *k) -> void {
        QtKitInformation::setQtVersion(k, versionData.qt);
        if (versionData.isTemporary)
            k->setValue(QT_IS_TEMPORARY, versionData.qt->uniqueId());

        k->setUnexpandedDisplayName(versionData.qt->displayName());;

        setup(k);
    });
}

} // namespace QtSupport
