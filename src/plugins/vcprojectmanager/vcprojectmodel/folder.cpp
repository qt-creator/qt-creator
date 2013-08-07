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

#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

Folder::Folder(VcProjectDocument *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc)
{
}

Folder::Folder(const Folder &folder)
{
    m_parentProjectDoc = folder.m_parentProjectDoc;
    m_name = folder.m_name;
    m_anyAttribute = folder.m_anyAttribute;

    foreach (const File::Ptr &file, folder.m_files)
        m_files.append(File::Ptr(new File(*file)));

    foreach (const Filter::Ptr &filter, folder.m_filters)
        m_filters.append(Filter::Ptr(new Filter(*filter)));

    foreach (const Folder::Ptr &fold, folder.m_folders)
        m_folders.append(Folder::Ptr(new Folder(*fold)));
}

Folder &Folder::operator =(const Folder &folder)
{
    if (this != &folder) {
        m_parentProjectDoc = folder.m_parentProjectDoc;
        m_name = folder.m_name;
        m_anyAttribute = folder.m_anyAttribute;

        m_files.clear();
        m_folders.clear();
        m_filters.clear();

        foreach (const File::Ptr &file, folder.m_files)
            m_files.append(File::Ptr(new File(*file)));

        foreach (const Filter::Ptr &filter, folder.m_filters)
            m_filters.append(Filter::Ptr(new Filter(*filter)));

        foreach (const Folder::Ptr &fold, folder.m_folders)
            m_folders.append(Folder::Ptr(new Folder(*fold)));
    }

    return *this;
}

Folder::~Folder()
{
}

void Folder::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

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

VcNodeWidget *Folder::createSettingsWidget()
{
    return 0;
}

QDomNode Folder::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Folder"));

    fileNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        fileNode.setAttribute(it.key(), it.value());
    }

    foreach (const File::Ptr &file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (const Filter::Ptr &filter, m_filters)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    foreach (const Folder::Ptr &folder, m_folders)
        fileNode.appendChild(folder->toXMLDomNode(domXMLDocument));

    return fileNode;
}

void Folder::addFilter(Filter::Ptr filter)
{
    if (m_filters.contains(filter))
        return;

    foreach (const Filter::Ptr &filt, m_filters) {
        if (filt->name() == filter->name())
            return;
    }

    m_filters.append(filter);
}

void Folder::removeFilter(Filter::Ptr filter)
{
    m_filters.removeAll(filter);
}

void Folder::removeFilter(const QString &filterName)
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName) {
            removeFilter(filter);
            return;
        }
    }
}

QList<Filter::Ptr> Folder::filters() const
{
    return m_filters;
}

Filter::Ptr Folder::filter(const QString &filterName) const
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName)
            return filter;
    }
    return Filter::Ptr();
}

void Folder::addFile(File::Ptr file)
{
    if (m_files.contains(file))
        return;

    foreach (const File::Ptr &f, m_files) {
        if (f->relativePath() == file->relativePath())
            return;
    }
    m_files.append(file);
}

void Folder::removeFile(File::Ptr file)
{
    m_files.removeAll(file);
}

void Folder::removeFile(const QString &relativeFilePath)
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativeFilePath) {
            removeFile(file);
            return;
        }
    }
}

QList<File::Ptr> Folder::files() const
{
    return m_files;
}

File::Ptr Folder::file(const QString &relativeFilePath) const
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativeFilePath)
            return file;
    }
    return File::Ptr();
}

bool Folder::fileExists(const QString &relativeFilePath)
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

void Folder::addFolder(Folder::Ptr folder)
{
    if (m_folders.contains(folder))
        return;

    foreach (const Folder::Ptr &f, m_folders) {
        if (f->name() == folder->name())
            return;
    }
    m_folders.append(folder);
}

void Folder::removeFolder(Folder::Ptr folder)
{
    m_folders.removeAll(folder);
}

void Folder::removeFolder(const QString &folderName)
{
    foreach (const Folder::Ptr &f, m_folders) {
        if (f->name() == folderName) {
            removeFolder(f);
            return;
        }
    }
}

QList<Folder::Ptr> Folder::folders() const
{
    return m_folders;
}

Folder::Ptr Folder::folder(const QString &folderName) const
{
    foreach (const Folder::Ptr &folder, m_folders) {
        if (folder->name() == folderName)
            return folder;
    }
    return Folder::Ptr();
}

QString Folder::name() const
{
    return m_name;
}

void Folder::setName(const QString &name)
{
    m_name = name;
}

QString Folder::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void Folder::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void Folder::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void Folder::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

void Folder::allFiles(QStringList &sl)
{
    foreach (const Filter::Ptr &filter, m_filters)
        filter->allFiles(sl);

    foreach (const Folder::Ptr &filter, m_folders)
        filter->allFiles(sl);

    foreach (const File::Ptr &file, m_files)
        sl.append(file->canonicalPath());
}

void Folder::processFile(const QDomNode &fileNode)
{
    File::Ptr file(new File(m_parentProjectDoc));
    file->processNode(fileNode);
    m_files.append(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Folder::processFilter(const QDomNode &filterNode)
{
    Filter::Ptr filter(new Filter(m_parentProjectDoc));
    filter->processNode(filterNode);
    m_filters.append(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Folder::processFolder(const QDomNode &folderNode)
{
    Folder::Ptr folder(new Folder(m_parentProjectDoc));
    folder->processNode(folderNode);
    m_folders.append(folder);

    // process next sibling
    QDomNode nextSibling = folderNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else if (nextSibling.nodeName() == QLatin1String("Folder"))
            processFolder(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Folder::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                setName(domElement.value());

            else
                setAttribute(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
