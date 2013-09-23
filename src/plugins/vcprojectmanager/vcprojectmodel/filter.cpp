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
#include "filter.h"

#include <QFileInfo>

#include "vcprojectdocument.h"
#include "generalattributecontainer.h"

namespace VcProjectManager {
namespace Internal {

Filter::Filter(const QString &containerType, VcProjectDocument *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc),
      m_containerType(containerType)
{
    m_attributeContainer = new GeneralAttributeContainer;
}

Filter::Filter(const Filter &filter)
{
    m_parentProjectDoc = filter.m_parentProjectDoc;
    m_name = filter.m_name;
    m_containerType = filter.m_containerType;
    m_attributeContainer = new GeneralAttributeContainer;
    *(m_attributeContainer) = *(filter.m_attributeContainer);

    foreach (IFile *file, filter.m_files)
        m_files.append(file->clone());

    foreach (IFileContainer *filt, filter.m_fileContainers)
        m_fileContainers.append(filt->clone());
}

Filter &Filter::operator =(const Filter &filter)
{
    if (this != &filter) {
        m_name = filter.m_name;
        m_parentProjectDoc = filter.m_parentProjectDoc;
        m_containerType = filter.m_containerType;
        *(m_attributeContainer) = *(filter.m_attributeContainer);

        m_files.clear();
        m_fileContainers.clear();

        foreach (IFile *file, filter.m_files)
            m_files.append(file->clone());

        foreach (IFileContainer *filt, filter.m_fileContainers)
            m_fileContainers.append(filt->clone());
    }
    return *this;
}

Filter::~Filter()
{
    delete m_attributeContainer;
}

QString Filter::containerType() const
{
    return m_containerType;
}

void Filter::processNode(const QDomNode &node)
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
        }
    }
}

VcNodeWidget *Filter::createSettingsWidget()
{
    return 0;
}

QDomNode Filter::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Filter"));
    fileNode.setAttribute(QLatin1String("Name"), m_name);

    m_attributeContainer->appendToXMLNode(fileNode);

    foreach (IFile *file, m_files)
        fileNode.appendChild(file->toXMLDomNode(domXMLDocument));

    foreach (IFileContainer *filter, m_fileContainers)
        fileNode.appendChild(filter->toXMLDomNode(domXMLDocument));

    return fileNode;
}

QString Filter::name() const
{
    return m_name;
}

void Filter::setName(const QString &name)
{
    m_name = name;
}

void Filter::addFile(IFile *file)
{
    if (m_files.contains(file))
        return;

    foreach (IFile *f, m_files) {
        if (f->relativePath() == file->relativePath())
            return;
    }

    m_files.append(file);
}

void Filter::removeFile(IFile *file)
{
    m_files.removeAll(file);
}

void Filter::removeFile(const QString &relativeFilePath)
{
    foreach (IFile *file, m_files) {
        if (file->relativePath() == relativeFilePath) {
            removeFile(file);
            delete file;
            return;
        }
    }
}

IFile* Filter::file(const QString &relativePath) const
{
    foreach (IFile *file, m_files) {
        if (file->relativePath() == relativePath)
            return file;
    }
    return 0;
}

QList<IFile *> Filter::files() const
{
    return m_files;
}

IFile *Filter::file(int index) const
{
    if (0 <= index && index < m_files.size())
        return m_files[index];
    return 0;
}

int Filter::fileCount() const
{
    return m_files.size();
}

void Filter::addFileContainer(IFileContainer *fileContainer)
{
    if (!fileContainer && m_fileContainers.contains(fileContainer))
        return;

    m_fileContainers.append(fileContainer);
}

int Filter::childCount() const
{
    return m_fileContainers.size();
}

IFileContainer *Filter::fileContainer(int index) const
{
    if (0 <= index && index < m_fileContainers.size())
        return m_fileContainers[index];
    return 0;
}

void Filter::removeFileContainer(IFileContainer *fileContainer)
{
    m_fileContainers.removeAll(fileContainer);
}

IAttributeContainer *Filter::attributeContainer() const
{
    return m_attributeContainer;
}

bool Filter::fileExists(const QString &relativeFilePath) const
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

void Filter::allFiles(QStringList &sl) const
{
    foreach (IFileContainer *filter, m_fileContainers)
        filter->allFiles(sl);

    foreach (IFile *file, m_files)
        sl.append(file->canonicalPath());
}

IFileContainer *Filter::clone() const
{
    return new Filter(*this);
}

void Filter::processFile(const QDomNode &fileNode)
{
    IFile *file = new File(m_parentProjectDoc);
    file->processNode(fileNode);
    addFile(file);

    // process next sibling
    QDomNode nextSibling = fileNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Filter::processFilter(const QDomNode &filterNode)
{
    IFileContainer *filter = new Filter(QLatin1String("Filter"), m_parentProjectDoc);
    filter->processNode(filterNode);
    addFileContainer(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else
            processFilter(nextSibling);
    }
}

void Filter::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
