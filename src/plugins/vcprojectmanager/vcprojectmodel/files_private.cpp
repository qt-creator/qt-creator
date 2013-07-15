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
#include "files_private.h"

#include "filefactory.h"
#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

Files_Private::~Files_Private()
{
    m_filters.clear();
}

void Files_Private::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull()) {
            if (firstChild.nodeName() == QLatin1String("Filter"))
                processFilter(firstChild);
            else if (firstChild.nodeName() == QLatin1String("File"))
                processFile(firstChild);
        }
    }
}

void Files_Private::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

QDomNode Files_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Files"));

    foreach (const File::Ptr &file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (const Filter::Ptr &filter, m_filters)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    return fileNode;
}

void Files_Private::addFilter(Filter::Ptr newFilter)
{
    if (m_filters.contains(newFilter))
        return;

    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == newFilter->name())
            return;
    }

    m_filters.append(newFilter);
}

void Files_Private::removeFilter(Filter::Ptr filter)
{
    m_filters.removeAll(filter);
}

QList<Filter::Ptr> Files_Private::filters() const
{
    return m_filters;
}

Filter::Ptr Files_Private::filter(const QString &filterName) const
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName)
            return filter;
    }
    return Filter::Ptr();
}

void Files_Private::addFile(File::Ptr newFile)
{
    if (m_files.contains(newFile))
        return;

    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == newFile->relativePath())
            return;
    }

    m_files.append(newFile);
}

void Files_Private::removeFile(File::Ptr file)
{
    m_files.removeAll(file);
}

QList<File::Ptr> Files_Private::files() const
{
    return m_files;
}

File::Ptr Files_Private::file(const QString &relativePath) const
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativePath)
            return file;
    }
    return File::Ptr();
}

Files_Private::Files_Private(VcProjectDocument *parentProject)
    : m_parentProject(parentProject)
{
}

Files_Private::Files_Private(const Files_Private &filesPrivate)
{
    foreach (const File::Ptr &file, filesPrivate.m_files)
        m_files.append(File::Ptr(new File(*file)));

    foreach (const Filter::Ptr &filter, filesPrivate.m_filters)
        m_filters.append(Filter::Ptr(new Filter(*filter)));
}

Files_Private &Files_Private::operator=(const Files_Private &filesPrivate)
{
    if (this != &filesPrivate) {
        m_files.clear();
        m_filters.clear();

        foreach (const File::Ptr &file, filesPrivate.m_files)
            m_files.append(File::Ptr(new File(*file)));

        foreach (const Filter::Ptr &filter, filesPrivate.m_filters)
            m_filters.append(Filter::Ptr(new Filter(*filter)));
    }
    return *this;
}

void Files_Private::processFile(const QDomNode &fileNode)
{
    File::Ptr file = FileFactory::createFile(m_parentProject);
    file->processNode(fileNode);
    m_files.append(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Files_Private::processFilter(const QDomNode &filterNode)
{
    Filter::Ptr filter(new Filter(m_parentProject));
    filter->processNode(filterNode);
    m_filters.append(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else
            processFilter(nextSibling);
    }
}


Files2003_Private::~Files2003_Private()
{
}

QSharedPointer<Files_Private> Files2003_Private::clone() const
{
    return QSharedPointer<Files_Private>(new Files2003_Private(*this));
}

Files2003_Private::Files2003_Private(VcProjectDocument *parentProjectDocument)
    : Files_Private(parentProjectDocument)
{
}

Files2003_Private::Files2003_Private(const Files2003_Private &filesPrivate)
    : Files_Private(filesPrivate)
{
}

Files2003_Private &Files2003_Private::operator=(const Files2003_Private &filesPrivate)
{
    if (this != &filesPrivate)
        Files_Private::operator =(filesPrivate);
    return *this;
}


Files2005_Private::Files2005_Private(VcProjectDocument *parentProjectDocument)
    : Files_Private(parentProjectDocument)
{
}

Files2005_Private::Files2005_Private(const Files2005_Private &filesPrivate)
    : Files_Private(filesPrivate)
{
    foreach (const Folder::Ptr &folder, filesPrivate.m_folders)
        m_folders.append(Folder::Ptr(new Folder(*folder)));
}

Files2005_Private &Files2005_Private::operator =(const Files2005_Private &filesPrivate)
{
    if (this != &filesPrivate) {
        Files_Private::operator =(filesPrivate);
        m_folders.clear();

        foreach (const Folder::Ptr &folder, filesPrivate.m_folders)
            m_folders.append(Folder::Ptr(new Folder(*folder)));
    }
    return *this;
}

Files2005_Private::~Files2005_Private()
{
}

void Files2005_Private::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();

        if (!firstChild.isNull()) {
            if (firstChild.nodeName() == QLatin1String("Filter"))
                processFilter(firstChild);
            else if (firstChild.nodeName() == QLatin1String("File"))
                processFile(firstChild);
            else if (firstChild.nodeName() == QLatin1String("Folder"))
                processFolder(firstChild);
        }
    }
}

QDomNode Files2005_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Files"));

    foreach (const File::Ptr &file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (const Filter::Ptr &filter, m_filters)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    foreach (const Folder::Ptr &folder, m_folders)
        fileNode.appendChild(folder->toXMLDomNode(domXMLDocument));

    return fileNode;
}

QSharedPointer<Files_Private> Files2005_Private::clone() const
{
    return QSharedPointer<Files_Private>(new Files2005_Private(*this));
}

void Files2005_Private::addFolder(Folder::Ptr newFolder)
{
    if (m_folders.contains(newFolder))
        return;

    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == newFolder->name())
            return;
    }

    m_folders.append(newFolder);
}

void Files2005_Private::removeFolder(Folder::Ptr folder)
{
    m_folders.removeAll(folder);
}

QList<Folder::Ptr> Files2005_Private::folders() const
{
    return m_folders;
}

Folder::Ptr Files2005_Private::folder(const QString &folderName) const
{
    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == folderName)
            return folder;
    }
    return Folder::Ptr();
}

void Files2005_Private::processFile(const QDomNode &fileNode)
{
    File::Ptr file = FileFactory::createFile(m_parentProject);
    file->processNode(fileNode);
    m_files.append(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Filter"))
            processFilter(nextSibling);
        else
            processFolder(nextSibling);
    }
}

void Files2005_Private::processFilter(const QDomNode &filterNode)
{
    Filter::Ptr filter(new Filter(m_parentProject));
    filter->processNode(filterNode);
    m_filters.append(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Filter"))
            processFilter(nextSibling);
        else
            processFolder(nextSibling);
    }
}

void Files2005_Private::processFolder(const QDomNode &folderNode)
{
    Folder::Ptr folder(new Folder(m_parentProject));
    folder->processNode(folderNode);
    m_folders.append(folder);

    // process next sibling
    QDomNode nextSibling = folderNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Filter"))
            processFilter(nextSibling);
        else
            processFolder(nextSibling);
    }
}


Files2008_Private::~Files2008_Private()
{
}

QSharedPointer<Files_Private> Files2008_Private::clone() const
{
    return QSharedPointer<Files_Private>(new Files2008_Private(*this));
}

Files2008_Private::Files2008_Private(VcProjectDocument *parentProjectDocument)
    : Files_Private(parentProjectDocument)
{
}

Files2008_Private::Files2008_Private(const Files2008_Private &filesPrivate)
    : Files_Private(filesPrivate)
{
}

Files2008_Private &Files2008_Private::operator =(const Files2008_Private &filesPrivate)
{
    if (this != &filesPrivate)
        Files_Private::operator =(filesPrivate);
    return *this;
}

} // namespace Internal
} // namespace VcProjectManager
