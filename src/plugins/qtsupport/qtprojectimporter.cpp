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
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QList>

using namespace ProjectExplorer;

namespace QtSupport {

QtProjectImporter::QtProjectImporter(const QString &path) : ProjectImporter(path)
{
    useTemporaryKitInformation(QtKitInformation::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryQt(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryQt(k, vl); });
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
        result.isTemporary = hasKitWithTemporaryData(QtKitInformation::id(), qtId);
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
                                           const ProjectImporter::KitSetupFunction &additionalSetup) const
{
    return ProjectImporter::createTemporaryKit([&additionalSetup, &versionData, this](Kit *k) -> void {
        QtKitInformation::setQtVersion(k, versionData.qt);
        if (versionData.isTemporary)
            addTemporaryData(QtKitInformation::id(), versionData.qt->uniqueId(), k);

        k->setUnexpandedDisplayName(versionData.qt->displayName());;

        additionalSetup(k);
    });
}

static BaseQtVersion *versionFromVariant(const QVariant &v)
{
    bool ok;
    const int qtId = v.toInt(&ok);
    QTC_ASSERT(ok, return nullptr);
    return QtVersionManager::version(qtId);
}

void QtProjectImporter::cleanupTemporaryQt(Kit *k, const QVariantList &vl)
{
    Q_UNUSED(k);

    QTC_ASSERT(vl.count() == 1, return);
    BaseQtVersion *version = versionFromVariant(vl.at(0));
    QTC_ASSERT(version, return);
    QtVersionManager::removeVersion(version);
}

void QtProjectImporter::persistTemporaryQt(Kit *k, const QVariantList &vl)
{
    QTC_ASSERT(vl.count() == 1, return);
    BaseQtVersion *tmpVersion = versionFromVariant(vl.at(0));
    BaseQtVersion *actualVersion = QtKitInformation::qtVersion(k);

    // User changed Kit away from temporary Qt that was set up:
    if (tmpVersion && actualVersion != tmpVersion)
        QtVersionManager::removeVersion(tmpVersion);
}

} // namespace QtSupport
