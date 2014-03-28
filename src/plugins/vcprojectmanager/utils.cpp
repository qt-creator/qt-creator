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
#include "utils.h"
#include "vcschemamanager.h"

#include <QFile>
#include <QXmlSchema>
#include <QXmlSchemaValidator>

namespace VcProjectManager {
namespace Internal {
namespace Utils {

bool checkIfVersion2003(const QString &filePath)
{
    VcSchemaManager *schemaMgr = VcSchemaManager::instance();
    QString vc2003Schema = schemaMgr->documentSchema(Constants::SV_2003);

    if (vc2003Schema.isEmpty()) {
        return false;
    }

    QFile schemaFile(vc2003Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QFile file(filePath);
        file.open(QIODevice::ReadOnly);

        QXmlSchemaValidator validator(schema);
        if (validator.validate(&file, QUrl::fromLocalFile(file.fileName())))
            return true;
    }

    return false;
}

bool checkIfVersion2005(const QString &filePath)
{
    VcSchemaManager *schemaMgr = VcSchemaManager::instance();
    QString vc2005Schema = schemaMgr->documentSchema(Constants::SV_2005);
    if (vc2005Schema.isEmpty())
        return false;

    QFile schemaFile(vc2005Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QFile file(filePath);
        file.open(QIODevice::ReadOnly);

        QXmlSchemaValidator validator(schema);
        if (validator.validate(&file, QUrl::fromLocalFile(file.fileName())))
            return true;
    }

    return false;
}

bool checkIfVersion2008(const QString &filePath)
{
    VcSchemaManager *schemaMgr = VcSchemaManager::instance();
    QString vc2008Schema = schemaMgr->documentSchema(Constants::SV_2008);
    if (vc2008Schema.isEmpty())
        return false;

    QFile schemaFile(vc2008Schema);
    schemaFile.open(QIODevice::ReadOnly);

    QXmlSchema schema;
    schema.load(&schemaFile, QUrl::fromLocalFile(schemaFile.fileName()));

    if (schema.isValid()) {
        QFile file(filePath);
        file.open(QIODevice::ReadOnly);

        QXmlSchemaValidator validator(schema);
        if (validator.validate(&file, QUrl::fromLocalFile(file.fileName())))
            return true;
    }

    return false;
}

VcDocConstants::DocumentVersion getProjectVersion(const QString &projectPath)
{
    if (checkIfVersion2003(projectPath))
        return VcDocConstants::DV_MSVC_2003;
    else if (checkIfVersion2005(projectPath))
        return VcDocConstants::DV_MSVC_2005;
    else if (checkIfVersion2008(projectPath))
        return VcDocConstants::DV_MSVC_2008;

    return VcDocConstants::DV_UNRECOGNIZED;
}

} // namespace Utils
} // namespace Internal
} // namespace VcProjectManager
