/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "files.h"

#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

Files::~Files()
{
}

void Files::processNode(const QDomNode &node)
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

VcNodeWidget *Files::createSettingsWidget()
{
    return 0;
}

QDomNode Files::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Files"));

    foreach (const File::Ptr &file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (const Filter::Ptr &filter, m_filters)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    return fileNode;
}

bool Files::isEmpty() const
{
    return m_files.isEmpty() && m_filters.isEmpty();
}

void Files::addFilter(Filter::Ptr newFilter)
{
    if (m_filters.contains(newFilter))
        return;

    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == newFilter->name())
            return;
    }

    m_filters.append(newFilter);
}

void Files::removeFilter(Filter::Ptr filter)
{
    m_filters.removeAll(filter);
}

void Files::removeFilter(const QString &filterName)
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName) {
            removeFilter(filter);
            return;
        }
    }
}

QList<Filter::Ptr> Files::filters() const
{
    return m_filters;
}

Filter::Ptr Files::filter(const QString &filterName) const
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName)
            return filter;
    }
    return Filter::Ptr();
}

void Files::addFile(File::Ptr newFile)
{
    if (m_files.contains(newFile))
        return;

    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == newFile->relativePath())
            return;
    }

    m_files.append(newFile);
}

void Files::removeFile(File::Ptr file)
{
    m_files.removeAll(file);
}

QList<File::Ptr> Files::files() const
{
    return m_files;
}

File::Ptr Files::file(const QString &relativePath) const
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativePath)
            return file;
    }
    return File::Ptr();
}

bool Files::fileExists(const QString &relativeFilePath) const
{
    foreach (const File::Ptr &filePtr, m_files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    foreach (const Filter::Ptr &filterPtr, m_filters) {
        if (filterPtr->fileExists(relativeFilePath))
            return true;
    }

    return false;
}

void Files::allProjectFiles(QStringList &sl) const
{
    foreach (const Filter::Ptr &filter, m_filters)
        filter->allFiles(sl);

    foreach (const File::Ptr &file, m_files)
        sl.append(file->canonicalPath());
}

Files::Files(VcProjectDocument *parentProject)
    : m_parentProject(parentProject)
{
}

Files::Files(const Files &files)
{
    m_files.clear();
    m_filters.clear();

    foreach (const File::Ptr &file, files.m_files)
        m_files.append(File::Ptr(new File(*file)));

    foreach (const Filter::Ptr &filter, files.m_filters)
        m_filters.append(Filter::Ptr(new Filter(*filter)));
}

Files &Files::operator=(const Files &files)
{
    if (this != &files) {
        m_files.clear();
        m_filters.clear();

        foreach (const File::Ptr &file, files.m_files)
            m_files.append(File::Ptr(new File(*file)));

        foreach (const Filter::Ptr &filter, files.m_filters)
            m_filters.append(Filter::Ptr(new Filter(*filter)));
    }
    return *this;
}

void Files::processFile(const QDomNode &fileNode)
{
    File::Ptr file(new File(m_parentProject));
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

void Files::processFilter(const QDomNode &filterNode)
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


Files2003::Files2003(VcProjectDocument *parentProjectDocument)
    : Files(parentProjectDocument)
{
}

Files2003::Files2003(const Files2003 &files)
    : Files(files)
{
}

Files2003 &Files2003::operator =(const Files2003 &files)
{
    if (this != &files)
        Files::operator =(files);
    return *this;
}

Files2003::~Files2003()
{
}

Files *Files2003::clone() const
{
    return new Files2003(*this);
}


Files2005::Files2005(VcProjectDocument *parentProjectDocument)
    : Files(parentProjectDocument)
{
}

Files2005::Files2005(const Files2005 &files)
    : Files(files)
{
    m_folders.clear();

    foreach (const Folder::Ptr &folder, files.m_folders)
        m_folders.append(Folder::Ptr(new Folder(*folder)));
}

Files2005 &Files2005::operator=(const Files2005 &files)
{
    if (this != &files) {
        Files::operator =(files);
        m_folders.clear();

        foreach (const Folder::Ptr &folder, files.m_folders)
            m_folders.append(Folder::Ptr(new Folder(*folder)));
    }
    return *this;
}

Files2005::~Files2005()
{
}

void Files2005::processNode(const QDomNode &node)
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

QDomNode Files2005::toXMLDomNode(QDomDocument &domXMLDocument) const
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

bool Files2005::isEmpty() const
{
    return Files::isEmpty() && m_folders.isEmpty();
}

Files *Files2005::clone() const
{
    return new Files2005(*this);
}

bool Files2005::fileExists(const QString &relativeFilePath) const
{
    foreach (const File::Ptr &filePtr, m_files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    foreach (const Filter::Ptr &filterPtr, m_filters) {
        if (filterPtr->fileExists(relativeFilePath))
            return true;
    }

    foreach (const Folder::Ptr &folderPtr, m_folders) {
        if (folderPtr->fileExists(relativeFilePath))
            return true;
    }

    return false;
}

void Files2005::addFolder(Folder::Ptr newFolder)
{
    if (m_folders.contains(newFolder))
        return;

    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == newFolder->name())
            return;
    }

    m_folders.append(newFolder);
}

void Files2005::removeFolder(Folder::Ptr folder)
{
    m_folders.removeAll(folder);
}

void Files2005::removeFolder(const QString &folderName)
{
    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == folderName) {
            removeFolder(folder);
            return;
        }
    }
}

QList<Folder::Ptr> Files2005::folders() const
{
    return m_folders;
}

Folder::Ptr Files2005::folder(const QString &folderName) const
{
    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == folderName)
            return folder;
    }
    return Folder::Ptr();
}

void Files2005::allProjectFiles(QStringList &sl) const
{
    foreach (const Filter::Ptr &filter, m_filters)
        filter->allFiles(sl);

    foreach (const Folder::Ptr &filter, m_folders)
        filter->allFiles(sl);

    foreach (const File::Ptr &file, m_files)
        sl.append(file->canonicalPath());
}

void Files2005::processFile(const QDomNode &fileNode)
{
    File::Ptr file(new File(m_parentProject));
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

void Files2005::processFilter(const QDomNode &filterNode)
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

void Files2005::processFolder(const QDomNode &folderNode)
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


Files2008::Files2008(VcProjectDocument *parentProjectDocument)
    : Files(parentProjectDocument)
{
}

Files2008::Files2008(const Files2008 &files)
    : Files(files)
{
}

Files2008 &Files2008::operator=(const Files2008 &files)
{
    if (this != &files)
        Files::operator =(files);
    return *this;
}

Files2008::~Files2008()
{
}

Files *Files2008::clone() const
{
    return new Files2008(*this);
}

} // namespace Internal
} // namespace VcProjectManager
