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
#include "folder.h"

#include <QFileInfo>

#include "filefactory.h"
#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

Folder::Folder(VcProjectDocument *parentProjectDoc)
    : m_folderType(QSharedPointer<FolderType>(new FolderType(parentProjectDoc)))
{
}

Folder::Folder(const Folder &folder)
{
    m_folderType = folder.m_folderType->clone();
}

Folder &Folder::operator =(const Folder &folder)
{
    if (this != &folder)
        m_folderType = folder.m_folderType->clone();
    return *this;
}

Folder::~Folder()
{
}

void Folder::processNode(const QDomNode &node)
{
    m_folderType->processNode(node);
}

void Folder::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *Folder::createSettingsWidget()
{
    return 0;
}

QDomNode Folder::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_folderType->toXMLDomNode(domXMLDocument);
}

void Folder::addFilter(Filter::Ptr filter)
{
    m_folderType->addFilter(filter);
}

void Folder::removeFilter(Filter::Ptr filterName)
{
    m_folderType->removeFilter(filterName);
}

void Folder::removeFilter(const QString &filterName)
{
    m_folderType->removeFilter(filterName);
}

QList<Filter::Ptr > Folder::filters() const
{
    return m_folderType->filters();
}

Filter::Ptr Folder::filter(const QString &filterName) const
{
    return m_folderType->filter(filterName);
}

void Folder::addFile(File::Ptr file)
{
    m_folderType->addFile(file);
}

void Folder::removeFile(File::Ptr file)
{
    m_folderType->removeFile(file);
}

void Folder::removeFile(const QString &relativeFilePath)
{
    m_folderType->removeFile(relativeFilePath);
}

QList<File::Ptr > Folder::files() const
{
    return m_folderType->files();
}

File::Ptr Folder::file(const QString &relativeFilePath) const
{
    return m_folderType->file(relativeFilePath);
}

bool Folder::fileExists(const QString &relativeFilePath)
{
    QList<File::Ptr> files = m_folderType->files();

    foreach (const File::Ptr &filePtr, files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    QList<Filter::Ptr> filters = m_folderType->filters();

    foreach (const Filter::Ptr &filterPtr, filters) {
        if (filterPtr->fileExists(relativeFilePath))
            return true;
    }

    QList<Folder::Ptr> folders = m_folderType->folders();

    foreach (const Folder::Ptr &folderPtr, folders) {
        if (folderPtr->fileExists(relativeFilePath))
            return true;
    }

    return false;
}

void Folder::addFolder(Folder::Ptr folder)
{
    m_folderType->addFolder(folder);
}

void Folder::removeFolder(Folder::Ptr folder)
{
    m_folderType->removeFolder(folder);
}

void Folder::removeFolder(const QString &folderName)
{
    m_folderType->removeFolder(folderName);
}

QList<Folder::Ptr > Folder::folders() const
{
    return m_folderType->folders();
}

Folder::Ptr Folder::folder(const QString &folderName) const
{
    return m_folderType->folder(folderName);
}

QString Folder::name() const
{
    return m_folderType->name();
}

void Folder::setName(const QString &name)
{
    m_folderType->setName(name);
}

QString Folder::attributeValue(const QString &attributeName) const
{
    return m_folderType->attributeValue(attributeName);
}

void Folder::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_folderType->setAttribute(attributeName, attributeValue);
}

void Folder::clearAttribute(const QString &attributeName)
{
    m_folderType->clearAttribute(attributeName);
}

void Folder::removeAttribute(const QString &attributeName)
{
    m_folderType->removeAttribute(attributeName);
}

void Folder::allFiles(QStringList &sl)
{
    QList<Folder::Ptr > folders = m_folderType->folders();
    QList<Filter::Ptr > filters = m_folderType->filters();
    QList<File::Ptr > files = m_folderType->files();

    foreach (const Filter::Ptr &filter, filters)
        filter->allFiles(sl);

    foreach (const Folder::Ptr &filter, folders)
        filter->allFiles(sl);

    foreach (const File::Ptr &file, files)
        sl.append(file->canonicalPath());
}

} // namespace Internal
} // namespace VcProjectManager
