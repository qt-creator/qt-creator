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
#include "vcprojectmanager.h"

#include "vcproject.h"
#include "vcprojectbuildoptionspage.h"
#include "vcprojectmanagerconstants.h"
#include "vcprojectmodel/vcprojectdocument_constants.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QAction>
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>

using namespace ProjectExplorer;

namespace VcProjectManager {
namespace Internal {

VcManager::VcManager(VcProjectBuildOptionsPage *configPage) :
    m_configPage(configPage)
{
    readSchemaPath();
}

QString VcManager::mimeType() const
{
    return QLatin1String(Constants::VCPROJ_MIMETYPE);
}

ProjectExplorer::Project *VcManager::openProject(const QString &fileName, QString *errorString)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    // Check whether the project file exists.
    if (canonicalFilePath.isEmpty()) {
        if (errorString)
            *errorString = tr("Failed opening project '%1': Project file does not exist").
                arg(QDir::toNativeSeparators(fileName));
        return 0;
    }

    // Check whether the project is already open or not.
    ProjectExplorerPlugin *projectExplorer = ProjectExplorerPlugin::instance();
    foreach (Project *pi, projectExplorer->session()->projects()) {
        if (canonicalFilePath == pi->document()->fileName()) {
            *errorString = tr("Failed opening project '%1': Project already open").
                    arg(QDir::toNativeSeparators(canonicalFilePath));
            return 0;
        }
    }

    // check if project is a valid vc project
    // versions supported are 2003, 2005 and 2008
    VcDocConstants::DocumentVersion docVersion = VcDocConstants::DV_UNRECOGNIZED;

    if (checkIfVersion2003(canonicalFilePath))
        docVersion = VcDocConstants::DV_MSVC_2003;
    else if (checkIfVersion2005(canonicalFilePath))
        docVersion = VcDocConstants::DV_MSVC_2005;
    else if (checkIfVersion2008(canonicalFilePath))
        docVersion = VcDocConstants::DV_MSVC_2008;

    if (docVersion != VcDocConstants::DV_UNRECOGNIZED)
        return new VcProject(this, canonicalFilePath, docVersion);

    qDebug() << "VcManager::openProject: Unrecognized file version";
    return 0;
}

void VcManager::updateContextMenu(Project *project, ProjectExplorer::Node *node)
{
    Q_UNUSED(node);
    m_contextProject = project;
}

bool VcManager::checkIfVersion2003(const QString &filePath) const
{
    if (m_vc2003Schema.isEmpty()) {
        return false;
    }

    QFile schemaFile(m_vc2003Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QXmlSchemaValidator validator( schema );
        if (validator.validate(QUrl(filePath)))
            return true;

        else
            return false;
    }

    return false;
}

bool VcManager::checkIfVersion2005(const QString &filePath) const
{
    if (m_vc2005Schema.isEmpty())
        return false;

    QFile schemaFile(m_vc2005Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QXmlSchemaValidator validator( schema );
        if (validator.validate(QUrl(filePath)))
            return true;

        else
            return false;
    }

    return false;
}

bool VcManager::checkIfVersion2008(const QString &filePath) const
{
    if (m_vc2008Schema.isEmpty())
        return false;

    QFile schemaFile(m_vc2008Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QXmlSchemaValidator validator( schema );
        if (validator.validate(QUrl(filePath)))
            return true;

        else
            return false;
    }

    return false;
}

void VcManager::readSchemaPath()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SETTINGS_GROUP));
    QString msSchemaPathsData = settings->value(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SCHEMA_PATH)).toString();
    settings->endGroup();

    QStringList schemaPaths = msSchemaPathsData.split(QLatin1Char(';'));

    foreach (const QString &schema, schemaPaths) {
        QStringList schemaData = schema.split(QLatin1String("::"));
        if (schemaData.size() == 2) {
            if (schemaData[0] == QLatin1String(Constants::VC_PROJECT_SCHEMA_2003_QUIALIFIER))
                m_vc2003Schema = schemaData[1];
            else if (schemaData[0] == QLatin1String(Constants::VC_PROJECT_SCHEMA_2005_QUIALIFIER))
                m_vc2005Schema = schemaData[1];
            else if (schemaData[0] == QLatin1String(Constants::VC_PROJECT_SCHEMA_2008_QUIALIFIER))
                m_vc2008Schema = schemaData[1];
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
