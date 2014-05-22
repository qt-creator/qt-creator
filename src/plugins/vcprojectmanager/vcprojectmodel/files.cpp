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
#include "file.h"
#include "filecontainer.h"
#include "files.h"
#include "vcprojectdocument.h"
#include "../vcprojectmanagerconstants.h"

#include <QDomNode>

namespace VcProjectManager {
namespace Internal {

Files::Files(IVisualStudioProject *parentProject)
    : m_parentProject(parentProject)
{
}

Files::Files(const Files &files)
{
    m_parentProject = files.m_parentProject;

    foreach (IFile *file, files.m_files) {
        if (file)
            m_files.append(file->clone());
    }

    foreach (IFileContainer *fileContainer, files.m_fileContainers) {
        if (fileContainer)
            m_fileContainers.append(fileContainer->clone());
    }
}

Files &Files::operator =(const Files &files)
{
    if (this != &files) {
        qDeleteAll(m_files);
        qDeleteAll(m_fileContainers);
        m_files.clear();
        m_fileContainers.clear();
        m_parentProject = files.m_parentProject;

        foreach (IFile *file, files.m_files) {
            if (file)
                m_files.append(file->clone());
        }

        foreach (IFileContainer *fileContainer, files.m_fileContainers) {
            if (fileContainer)
                m_fileContainers.append(fileContainer->clone());
        }
    }
    return *this;
}

Files::~Files()
{
    qDeleteAll(m_files);
    qDeleteAll(m_fileContainers);
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
            else if (firstChild.nodeName() == QLatin1String("Folder"))
                processFolder(firstChild);
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

    foreach (IFile *file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (IFileContainer *filter, m_fileContainers)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    return fileNode;
}

void Files::addFile(IFile *newFile)
{
    if (m_files.contains(newFile))
        return;

    foreach (IFile *file, m_files) {
        if (file->relativePath() == newFile->relativePath())
            return;
    }

    m_files.append(newFile);
}

void Files::removeFile(IFile *file)
{
    m_files.removeAll(file);
}

IFile* Files::file(const QString &relativePath) const
{
    foreach (IFile *file, m_files) {
        if (file->relativePath() == relativePath)
            return file;
    }
    return 0;
}

bool Files::fileExists(const QString &relativeFilePath) const
{
    foreach (IFile *filePtr, m_files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    foreach (IFileContainer *filterPtr, m_fileContainers) {
        if (filterPtr->fileExists(relativeFilePath))
            return true;
    }

    return false;
}

int Files::fileCount() const
{
    return m_files.size();
}

IFile *Files::file(int index) const
{
    if (0 <= index && index < m_files.size())
        return m_files[index];
    return 0;
}

void Files::addFileContainer(IFileContainer *fileContainer)
{
    if (!fileContainer || m_fileContainers.contains(fileContainer))
        return;

    foreach (IFileContainer *fileCont, m_fileContainers) {
        if (fileCont &&
                fileCont->containerType() == fileContainer->containerType() &&
                fileCont->displayName() == fileContainer->displayName())
            return;
    }

    m_fileContainers.append(fileContainer);
}

int Files::fileContainerCount() const
{
    return m_fileContainers.size();
}

IFileContainer *Files::fileContainer(int index) const
{
    if (0 <= index && index < m_fileContainers.size())
        return m_fileContainers[index];
    return 0;
}

void Files::removeFileContainer(IFileContainer *fileContainer)
{
    m_fileContainers.removeOne(fileContainer);
}

void Files::processFile(const QDomNode &fileNode)
{
    IFile *file = new File(m_parentProject);
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

void Files::processFilter(const QDomNode &filterNode)
{
    IFileContainer *filter = new FileContainer(QLatin1String("Filter"), m_parentProject);
    filter->processNode(filterNode);
    m_fileContainers.append(filter);

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

void Files::processFolder(const QDomNode &folderNode)
{
    IFileContainer *folder = new FileContainer(QLatin1String("Folder"), m_parentProject);
    folder->processNode(folderNode);
    m_fileContainers.append(folder);

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

} // namespace Internal
} // namespace VcProjectManager
