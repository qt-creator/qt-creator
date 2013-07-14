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
#include "msbuildversionmanager.h"

#include "vcprojectmanagerconstants.h"

#include <coreplugin/icore.h>
#include <QStringList>

namespace VcProjectManager {
namespace Internal {

MsBuildVersionManager *MsBuildVersionManager::m_instance = 0;

MsBuildVersionManager *MsBuildVersionManager::instance()
{
    return m_instance;
}

MsBuildVersionManager::~MsBuildVersionManager()
{
    qDeleteAll(m_msBuildInfos);
    m_msBuildInfos.clear();
}

bool MsBuildVersionManager::addMsBuildInformation(MsBuildInformation *msBuildInfo)
{
    if (!msBuildInfo)
        return false;

    foreach (MsBuildInformation *info, m_msBuildInfos) {
        if (info->m_executable == msBuildInfo->m_executable)
            return false;
    }

    m_msBuildInfos.append(msBuildInfo);
    emit msBuildAdded(msBuildInfo->getId());
    return true;
}

QList<MsBuildInformation*> MsBuildVersionManager::msBuildInformations() const
{
    return m_msBuildInfos;
}

MsBuildInformation *MsBuildVersionManager::msBuildInformation(Core::Id msBuildID)
{
    foreach (MsBuildInformation *info, m_msBuildInfos) {
        if (info->getId() == msBuildID)
            return info;
    }
    return 0;
}

MsBuildInformation *MsBuildVersionManager::msBuildInformation(MsBuildInformation::MsBuildVersion minVersion, MsBuildInformation::MsBuildVersion maxVersion)
{
    foreach (MsBuildInformation *info, m_msBuildInfos) {
        if (info->m_msBuildVersion >= minVersion && info->m_msBuildVersion <= maxVersion)
            return info;
    }
    return 0;
}

void MsBuildVersionManager::removeMsBuildInformation(const Core::Id &msBuildId)
{
    for (int i = 0; i < m_msBuildInfos.size(); ++i) {
        MsBuildInformation *info = m_msBuildInfos[i];
        if (info->getId() == msBuildId) {
            m_msBuildInfos.removeOne(info);
            emit msBuildRemoved(msBuildId);
            delete info;
            return;
        }
    }
}

void MsBuildVersionManager::replace(Core::Id targetMsBuild, MsBuildInformation *newMsBuild)
{
    MsBuildInformation *oldMsBuild = msBuildInformation(targetMsBuild);

    if (oldMsBuild) {
        int index = m_msBuildInfos.indexOf(oldMsBuild);
        m_msBuildInfos.replace(index, newMsBuild);
        delete oldMsBuild;
        emit msBuildReplaced(targetMsBuild, newMsBuild->getId());
    }
}

void MsBuildVersionManager::saveSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginWriteArray(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_INFORMATIONS));
    for (int i = 0; i < m_msBuildInfos.size(); ++i) {
        settings->setArrayIndex(i);
        settings->setValue(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_EXECUTABLE), m_msBuildInfos[i]->m_executable);
        settings->setValue(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_EXECUTABLE_VERSION), m_msBuildInfos[i]->m_versionString);
    }
    settings->endArray();
}

MsBuildInformation *MsBuildVersionManager::createMsBuildInfo(const QString &executablePath, const QString &version)
{
    MsBuildInformation *newMsBuild = new MsBuildInformation(executablePath, version);

    // check if there is already a ms build with the same id, collision detection
    MsBuildInformation *info = m_instance->msBuildInformation(newMsBuild->getId());
    int i = 0;

    // if there is a id collision continue to generate id until unique id is created
    while (info)
    {
        QString argument = QString::number(i);
        QString temp = newMsBuild->m_executable + newMsBuild->m_versionString + argument;
        Core::Id newId(temp.toStdString().c_str());
        newMsBuild->setId(newId);
        info = m_instance->msBuildInformation(newMsBuild->getId());
        ++i;
    }

    return newMsBuild;
}

MsBuildVersionManager::MsBuildVersionManager()
{
    m_instance = this;
    loadSettings();
}

void MsBuildVersionManager::loadSettings()
{
    qDeleteAll(m_msBuildInfos);
    m_msBuildInfos.clear();

    QSettings *settings = Core::ICore::settings();
    int size = settings->beginReadArray(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_INFORMATIONS));
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        MsBuildInformation *buildInfo = createMsBuildInfo(settings->value(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_EXECUTABLE)).toString(),
                                                          settings->value(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_EXECUTABLE_VERSION)).toString());
        m_msBuildInfos.append(buildInfo);
    }

    settings->endArray();
}

} // namespace Internal
} // namespace VcProjectManager
