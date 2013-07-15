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
#ifndef VCPROJECTMANAGER_INTERNAL_FILTERTYPE_H
#define VCPROJECTMANAGER_INTERNAL_FILTERTYPE_H

#include "file.h"

namespace VcProjectManager {
namespace Internal {

class Filter;

class FilterType
{
    friend class Filter;

public:
    typedef QSharedPointer<FilterType>  Ptr;

    ~FilterType();
    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    QString name() const;
    void setName(const QString &name);

    void addFilter(QSharedPointer<Filter> filter);
    void removeFilter(QSharedPointer<Filter> filter);
    void removeFilter(const QString &filterName);
    QList<QSharedPointer<Filter> > filters() const;

    void addFile(File::Ptr file);
    void removeFile(File::Ptr file);
    void removeFile(const QString &relativeFilePath);
    File::Ptr file(const QString &relativePath) const;
    QList<File::Ptr> files() const;

    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

    FilterType::Ptr clone() const;

private:
    FilterType(VcProjectDocument *parentProjectDoc);
    FilterType(const FilterType &filterType);
    FilterType& operator=(const FilterType &filterType);
    void processNodeAttributes(const QDomElement &element);
    void processFile(const QDomNode &fileNode);
    void processFilter(const QDomNode &filterNode);

    QString m_name;
    QHash<QString, QString> m_anyAttribute;
    QList<QSharedPointer<Filter> > m_filters;
    QList<File::Ptr> m_files;
    VcProjectDocument *m_parentProjectDoc;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FILTERTYPE_H
