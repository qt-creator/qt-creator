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
#include "vcschemamanager.h"

#include <coreplugin/icore.h>

#include <QSettings>
#include <QStringList>

namespace VcProjectManager {
namespace Internal {

VcSchemaManager* VcSchemaManager::m_instance = 0;

VcSchemaManager *VcSchemaManager::instance()
{
    return m_instance;
}

VcSchemaManager::~VcSchemaManager()
{
}

void VcSchemaManager::addSchema(const QString &schemaPath, Constants::SchemaVersion version)
{
    m_schemas.insert(version, schemaPath);
}

QString VcSchemaManager::schema(Constants::SchemaVersion version)
{
    return m_schemas.value(version);
}

void VcSchemaManager::setSchema(Constants::SchemaVersion version, const QString &schemaPath)
{
    m_schemas.insert(version, schemaPath);
}

void VcSchemaManager::removeSchema(Constants::SchemaVersion version)
{
    m_schemas.remove(version);
}

void VcSchemaManager::removeAllSchemas()
{
    m_schemas.clear();
}

void VcSchemaManager::saveSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginWriteArray(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SCHEMA_PATH));
    settings->setArrayIndex(0);

    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2003_QUIALIFIER),
                       m_schemas.value(Constants::SV_2003));
    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2005_QUIALIFIER),
                       m_schemas.value(Constants::SV_2005));
    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2008_QUIALIFIER),
                       m_schemas.value(Constants::SV_2008));
    settings->endArray();
}

VcSchemaManager::VcSchemaManager()
{
    m_instance = this;
    loadSettings();
}

void VcSchemaManager::loadSettings()
{
    m_schemas.clear();

    QSettings *settings = Core::ICore::settings();
    settings->beginReadArray(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SCHEMA_PATH));
    settings->setArrayIndex(0);
    m_schemas.insert(Constants::SV_2003, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2003_QUIALIFIER)).toString());
    m_schemas.insert(Constants::SV_2005, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2005_QUIALIFIER)).toString());
    m_schemas.insert(Constants::SV_2008, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2008_QUIALIFIER)).toString());
    settings->endArray();
}

} // namespace Internal
} // namespace VcProjectManager
