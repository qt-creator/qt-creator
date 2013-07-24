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
#include "filtertype.h"

#include "filter.h"

namespace VcProjectManager {
namespace Internal {

FilterType::~FilterType()
{
    m_filters.clear();
    m_files.clear();
}

void FilterType::processNode(const QDomNode &node)
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

QDomNode FilterType::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("Filter"));
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

    return fileNode;
}

QString FilterType::name() const
{
    return m_name;
}

void FilterType::setName(const QString &name)
{
    m_name = name;
}

void FilterType::addFilter(QSharedPointer<VcProjectManager::Internal::Filter> filter)
{
    if (m_filters.contains(filter))
        return;

    foreach (const Filter::Ptr &filt, m_filters) {
        if (filt->name() == filter->name())
            return;
    }

    m_filters.append(filter);
}

void FilterType::removeFilter(QSharedPointer<VcProjectManager::Internal::Filter> filter)
{
    m_filters.removeAll(filter);
}

void FilterType::removeFilter(const QString &filterName)
{
    foreach (const Filter::Ptr &filter, m_filters) {
        if (filter->name() == filterName) {
            removeFilter(filter);
            return;
        }
    }
}

QList<QSharedPointer<VcProjectManager::Internal::Filter> > FilterType::filters() const
{
    return m_filters;
}

void FilterType::addFile(File::Ptr file)
{
    if (m_files.contains(file))
        return;

    foreach (const File::Ptr &f, m_files) {
        if (f->relativePath() == file->relativePath())
            return;
    }

    m_files.append(file);
}

void FilterType::removeFile(File::Ptr file)
{
    m_files.removeAll(file);
}

void FilterType::removeFile(const QString &relativeFilePath)
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativeFilePath) {
            removeFile(file);
            return;
        }
    }
}

File::Ptr FilterType::file(const QString &relativePath) const
{
    foreach (const File::Ptr &file, m_files) {
        if (file->relativePath() == relativePath)
            return file;
    }
    return File::Ptr();
}

QList<File::Ptr> FilterType::files() const
{
    return m_files;
}

QString FilterType::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void FilterType::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void FilterType::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void FilterType::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

FilterType::Ptr FilterType::clone() const
{
    return FilterType::Ptr(new FilterType(*this));
}

FilterType::FilterType(VcProjectDocument *parentProjectDoc)
    : m_parentProjectDoc(parentProjectDoc)
{
}

FilterType::FilterType(const FilterType &filterType)
{
    m_parentProjectDoc = filterType.m_parentProjectDoc;
    m_anyAttribute = filterType.m_anyAttribute;
    m_name = filterType.m_name;

    foreach (const File::Ptr &file, filterType.m_files)
        m_files.append(File::Ptr(new File(*file)));

    foreach (const Filter::Ptr &filter, filterType.m_filters)
        m_filters.append(Filter::Ptr(new Filter(*filter)));
}

FilterType &FilterType::operator =(const FilterType &filterType)
{
    if (this != &filterType) {
        m_name = filterType.m_name;
        m_parentProjectDoc = filterType.m_parentProjectDoc;
        m_anyAttribute = filterType.m_anyAttribute;
        m_files.clear();
        m_filters.clear();

        foreach (const File::Ptr &file, filterType.m_files)
            m_files.append(File::Ptr(new File(*file)));

        foreach (const Filter::Ptr &file, filterType.m_filters)
            m_filters.append(Filter::Ptr(new Filter(*file)));
    }
    return *this;
}

void FilterType::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

void FilterType::processFile(const QDomNode &fileNode)
{
    File::Ptr file(new File(m_parentProjectDoc));
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

void FilterType::processFilter(const QDomNode &filterNode)
{
    Filter::Ptr filter(new Filter(m_parentProjectDoc));
    filter->processNode(filterNode);
    addFilter(filter);

    // process next sibling
    QDomNode nextSibling = filterNode.nextSibling();
    if (!nextSibling.isNull()) {
        if (nextSibling.nodeName() == QLatin1String("File"))
            processFile(nextSibling);
        else
            processFilter(nextSibling);
    }
}

} // namespace Internal
} // namespace VcProjectManager
