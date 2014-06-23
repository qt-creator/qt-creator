/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "projectmacroexpander.h"
#include "kit.h"
#include "kitinformation.h"
#include "projectexplorerconstants.h"

#include <coreplugin/variablemanager.h>
#include <ssh/sshconnection.h>

using namespace ProjectExplorer;

ProjectMacroExpander::ProjectMacroExpander(const QString &projectFilePath, const QString &projectName,
                                 const Kit *k, const QString &bcName)
    : m_projectFile(projectFilePath), m_projectName(projectName), m_kit(k), m_bcName(bcName)
{ }

bool ProjectMacroExpander::resolveProjectMacro(const QString &name, QString *ret)
{
    QString result;
    bool found = false;
    if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTPROJECT_NAME)) {
        if (!m_projectName.isEmpty()) {
            result = m_projectName;
            found = true;
        }
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_NAME)) {
        result = m_kit->displayName();
        found = true;
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_FILESYSTEMNAME)) {
        result = m_kit->fileSystemFriendlyName();
        found = true;
    } else if (m_kit && name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTKIT_ID)) {
        result = m_kit->id().toString();
        found = true;
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTBUILD_NAME)) {
        result = m_bcName;
        found = true;
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTDEVICE_HOSTADDRESS)) {
        const IDevice::ConstPtr device = DeviceKitInformation::device(m_kit);
        if (device) {
            result = device->sshParameters().host;
            found = true;
        }
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTDEVICE_SSHPORT)) {
        const IDevice::ConstPtr device = DeviceKitInformation::device(m_kit);
        if (device) {
            result = QString::number(device->sshParameters().port);
            found = true;
        }
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTDEVICE_USERNAME)) {
        const IDevice::ConstPtr device = DeviceKitInformation::device(m_kit);
        if (device) {
            result = device->sshParameters().userName;
            found = true;
        }
    } else if (name == QLatin1String(ProjectExplorer::Constants::VAR_CURRENTDEVICE_PRIVATEKEYFILE)) {
        const IDevice::ConstPtr device = DeviceKitInformation::device(m_kit);
        if (device) {
            result = device->sshParameters().privateKeyFile;
            found = true;
        }
    }
    if (ret)
        *ret = result;
    return found;
}

// Try to resolve using local information, otherwise fall back to global variables.
bool ProjectMacroExpander::resolveMacro(const QString &name, QString *ret)
{
    bool found = resolveProjectMacro(name, ret);
    if (!found) {
        QString result = Core::VariableManager::value(name.toUtf8(), &found);
        if (ret)
            *ret = result;
    }
    return found;
}
