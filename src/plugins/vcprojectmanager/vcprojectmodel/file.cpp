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
#include "file.h"

#include "fileconfiguration.h"
#include "filetype.h"
#include "fileconfigurationfactory.h"
#include "filefactory.h"

namespace VcProjectManager {
namespace Internal {

File::File(VcProjectDocument *parentProjectDoc)
    : m_fileType(FileType::Ptr(new FileType(parentProjectDoc)))
{
}

File::File(const File &file)
{
    m_fileType = file.m_fileType->clone();
}

File &File::operator =(const File &file)
{
    if (this != &file)
        m_fileType = file.m_fileType->clone();
    return *this;
}

File::~File()
{
}

void File::processNode(const QDomNode &node)
{
    m_fileType->processNode(node);
}

void File::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *File::createSettingsWidget()
{
    return 0;
}

QDomNode File::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_fileType->toXMLDomNode(domXMLDocument);
}

void File::addFile(File::Ptr file)
{
    m_fileType->addFile(file);
}

void File::removeFile(File::Ptr file)
{
    m_fileType->removeFile(file);
}

void File::addFileConfiguration(FileConfiguration::Ptr fileConfig)
{
    m_fileType->addFileConfiguration(fileConfig);
}

void File::removeFileConfiguration(FileConfiguration::Ptr fileConfig)
{
    m_fileType->removeFileConfiguration(fileConfig);
}

QString File::attributeValue(const QString &attributeName) const
{
    return m_fileType->attributeValue(attributeName);
}

void File::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_fileType->setAttribute(attributeName, attributeValue);
}

void File::clearAttribute(const QString &attributeName)
{
    m_fileType->clearAttribute(attributeName);
}

void File::removeAttribute(const QString &attributeName)
{
    m_fileType->removeAttribute(attributeName);
}

QString File::relativePath() const
{
    return m_fileType->relativePath();
}

void File::setRelativePath(const QString &relativePath)
{
    m_fileType->setRelativePath(relativePath);
}

ProjectExplorer::FileType File::fileType() const
{
    return  m_fileType->fileType();
}

QString File::canonicalPath() const
{
    return m_fileType->canonicalPath();
}

} // namespace Internal
} // namespace VcProjectManager
