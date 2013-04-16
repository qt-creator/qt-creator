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
#ifndef VCPROJECTMANAGER_INTERNAL_FOLDERTYPE_H
#define VCPROJECTMANAGER_INTERNAL_FOLDERTYPE_H

#include "file.h"
#include "filter.h"

namespace VcProjectManager {
namespace Internal {

class Folder;

class FolderType
{
    friend class Folder;

public:
    ~FolderType();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    void processFile(const QDomNode &fileNode);
    void processFilter(const QDomNode &filterNode);
    void processFolder(const QDomNode &folderNode);

    void addFilter(Filter::Ptr filter);
    void removeFilter(Filter::Ptr filter);
    void removeFilter(const QString &filterName);
    QList<Filter::Ptr > filters() const;
    Filter::Ptr filter(const QString &filterName) const;

    void addFile(File::Ptr file);
    void removeFile(File::Ptr file);
    void removeFile(const QString &relativeFilePath);
    QList<File::Ptr > files() const;
    File::Ptr file(const QString &relativeFilePath) const;

    void addFolder(QSharedPointer<Folder> folder);
    void removeFolder(QSharedPointer<Folder> folder);
    void removeFolder(const QString &folderName);
    QList<QSharedPointer<Folder> > folders() const;
    QSharedPointer<Folder> folder(const QString &folderName) const;

    QString name() const;
    void setName(const QString &name);

    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

    QSharedPointer<FolderType> clone() const;

private:
    FolderType(VcProjectDocument *parentProjectDoc);
    FolderType(const FolderType &folderType);
    FolderType& operator=(const FolderType &folderType);

    QList<QSharedPointer<Folder> > m_folders;
    QList<File::Ptr > m_files;
    QList<Filter::Ptr > m_filters;

    QString m_name; // required
    QHash<QString, QString> m_anyAttribute;
    VcProjectDocument *m_parentProjectDoc;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FOLDERTYPE_H
