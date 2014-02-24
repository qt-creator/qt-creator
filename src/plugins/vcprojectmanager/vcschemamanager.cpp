/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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

#include "vcprojectmanagerconstants.h"
#include "vcprojectmodel/tools/tool_constants.h"
#include "vcprojectmodel/tools/toolattributes/tooldescriptiondatamanager.h"

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

void VcSchemaManager::addDocumentSchema(const QString &schemaPath, Constants::SchemaVersion version)
{
    m_documentSchemas.insert(version, schemaPath);
}

QString VcSchemaManager::documentSchema(Constants::SchemaVersion version)
{
    return m_documentSchemas.value(version);
}

void VcSchemaManager::setDocumentSchema(Constants::SchemaVersion version, const QString &schemaPath)
{
    m_documentSchemas.insert(version, schemaPath);
}

void VcSchemaManager::removeDocumentSchema(Constants::SchemaVersion version)
{
    m_documentSchemas.remove(version);
}

QString VcSchemaManager::toolSchema() const
{
    return m_toolSchema;
}

void VcSchemaManager::setToolSchema(const QString &schemaPath)
{
    m_toolSchema = schemaPath;
}

QList<QString> VcSchemaManager::toolXMLFilePaths() const
{
    return m_toolXMLPaths.values();
}

void VcSchemaManager::addToolXML(const QString &toolKey, const QString &toolFilePath)
{
    if (m_toolXMLPaths.contains(toolKey))
        return;

    m_toolXMLPaths[toolKey] = toolFilePath;
}

void VcSchemaManager::removeToolXML(const QString &toolKey)
{
    m_toolXMLPaths.remove(toolKey);
}

void VcSchemaManager::removeToolSchemas()
{
    m_toolXMLPaths.clear();
}

void VcSchemaManager::removeAllSchemas()
{
    m_documentSchemas.clear();
    m_toolSchema.clear();
}

void VcSchemaManager::saveSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SCHEMA_PATH));

    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2003_QUIALIFIER),
                       m_documentSchemas.value(Constants::SV_2003));
    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2005_QUIALIFIER),
                       m_documentSchemas.value(Constants::SV_2005));
    settings->setValue(QLatin1String(Constants::VC_PROJECT_SCHEMA_2008_QUIALIFIER),
                       m_documentSchemas.value(Constants::SV_2008));

    settings->endGroup();

    settings->setValue(QLatin1String(Constants::VC_PROJECT_TOOL_SCHEMA), m_toolSchema);

    settings->beginGroup(QLatin1String(Constants::VC_PROJECT_TOOL_XML));

    QHashIterator<QString, QString> it(m_toolXMLPaths);

    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }

    settings->endGroup();
}

VcSchemaManager::VcSchemaManager()
{
    m_instance = this;
    loadSettings();
}

void VcSchemaManager::loadSettings()
{
    m_documentSchemas.clear();

    QSettings *settings = Core::ICore::settings();

    settings->beginGroup(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SCHEMA_PATH));
    m_documentSchemas.insert(Constants::SV_2003, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2003_QUIALIFIER)).toString());
    m_documentSchemas.insert(Constants::SV_2005, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2005_QUIALIFIER)).toString());
    m_documentSchemas.insert(Constants::SV_2008, settings->value(QLatin1String(Constants::VC_PROJECT_SCHEMA_2008_QUIALIFIER)).toString());
    settings->endGroup();

    m_toolSchema = settings->value(QLatin1String(Constants::VC_PROJECT_TOOL_SCHEMA)).toString();

    settings->beginGroup(QLatin1String(Constants::VC_PROJECT_TOOL_XML));
    QStringList childKeys = settings->childKeys();

    foreach (const QString &key, childKeys)
        m_toolXMLPaths[key] = settings->value(key).toString();

    settings->endGroup();
}

bool VcSchemaManager::similarToolXMLExistsFor(const QString &filePath)
{
    ToolInfo fileToolInfo = ToolDescriptionDataManager::readToolInfo(filePath);

    foreach (const QString &toolXMLPath, m_toolXMLPaths) {
        ToolInfo info = ToolDescriptionDataManager::readToolInfo(toolXMLPath);
        if (info == fileToolInfo)
            return true;
    }

    return false;
}

} // namespace Internal
} // namespace VcProjectManager
