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
#include "filter.h"

#include <QFileInfo>

#include "filefactory.h"
#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

Filter::Filter(VcProjectDocument *parentProjectDoc)
    : m_filterType(FilterType::Ptr(new FilterType(parentProjectDoc)))
{
}

Filter::Filter(const Filter &filter)
{
    m_filterType = filter.m_filterType->clone();
}

Filter &Filter::operator =(const Filter &filter)
{
    if (this != &filter)
        m_filterType = filter.m_filterType->clone();
    return *this;
}

Filter::~Filter()
{
}

void Filter::processNode(const QDomNode &node)
{
    m_filterType->processNode(node);
}

void Filter::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *Filter::createSettingsWidget()
{
    return 0;
}

QDomNode Filter::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_filterType->toXMLDomNode(domXMLDocument);
}

QString Filter::name() const
{
    return m_filterType->name();
}

void Filter::setName(const QString &name)
{
    m_filterType->setName(name);
}

void Filter::addFilter(Filter::Ptr filter)
{
    m_filterType->addFilter(filter);
}

void Filter::removeFilter(Filter::Ptr filter)
{
    m_filterType->removeFilter(filter);
}

void Filter::removeFilter(const QString &filterName)
{
    m_filterType->removeFilter(filterName);
}

QList<Filter::Ptr > Filter::filters() const
{
    return m_filterType->filters();
}

void Filter::addFile(File::Ptr file)
{
    m_filterType->addFile(file);
}

void Filter::removeFile(File::Ptr file)
{
    m_filterType->removeFile(file);
}

void Filter::removeFile(const QString &relativeFilePath)
{
    m_filterType->removeFile(relativeFilePath);
}

File::Ptr Filter::file(const QString &relativePath) const
{
    return m_filterType->file(relativePath);
}

QList<File::Ptr > Filter::files() const
{
    return m_filterType->files();
}

bool Filter::fileExists(const QString &relativeFilePath)
{
    QList<File::Ptr> files = m_filterType->files();

    foreach (File::Ptr filePtr, files) {
        if (filePtr->relativePath() == relativeFilePath)
            return true;
    }

    QList<Filter::Ptr> filters = m_filterType->filters();

    foreach (Filter::Ptr filterPtr, filters)
        if (filterPtr->fileExists(relativeFilePath))
            return true;

    return false;
}

QString Filter::attributeValue(const QString &attributeName) const
{
    return m_filterType->attributeValue(attributeName);
}

void Filter::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_filterType->setAttribute(attributeName, attributeValue);
}

void Filter::clearAttribute(const QString &attributeName)
{
    m_filterType->clearAttribute(attributeName);
}

void Filter::removeAttribute(const QString &attributeName)
{
    m_filterType->removeAttribute(attributeName);
}

void Filter::allFiles(QStringList &sl) const
{
    QList<Filter::Ptr > filters = m_filterType->filters();
    QList<File::Ptr > files = m_filterType->files();

    foreach (Filter::Ptr filter, filters)
        filter->allFiles(sl);

    foreach (File::Ptr file, files)
        sl.append(file->canonicalPath());
}

} // namespace Internal
} // namespace VcProjectManager
