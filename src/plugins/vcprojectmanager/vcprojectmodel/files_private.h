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
#ifndef VCPROJECTMANAGER_INTERNAL_FILES_PRIVATE_H
#define VCPROJECTMANAGER_INTERNAL_FILES_PRIVATE_H

#include "file.h"
#include "filter.h"
#include "folder.h"

namespace VcProjectManager {
namespace Internal {

class Files_Private
{
    friend class Files;

public:
    typedef QSharedPointer<Files_Private>   Ptr;

    virtual ~Files_Private();
    virtual void processNode(const QDomNode &node);
    virtual void processNodeAttributes(const QDomElement &element);
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a Files_Private instance.
     * \return A shared pointer to newly created Files_Private instance.
     */
    virtual QSharedPointer<Files_Private> clone() const = 0;

    void addFilter(Filter::Ptr newFilter);
    void removeFilter(Filter::Ptr filter);
    QList<Filter::Ptr> filters() const;
    Filter::Ptr filter(const QString &filterName) const;

    void addFile(File::Ptr file);
    void removeFile(File::Ptr file);
    QList<File::Ptr> files() const;
    File::Ptr file(const QString &relativePath) const;

protected:
    Files_Private(VcProjectDocument *parentProject);
    Files_Private(const Files_Private &filesPrivate);
    Files_Private& operator=(const Files_Private &filesPrivate);

    virtual void processFile(const QDomNode &fileNode);
    virtual void processFilter(const QDomNode &filterNode);

    QList<Filter::Ptr> m_filters;
    QList<File::Ptr> m_files;
    VcProjectDocument *m_parentProject;
};

class Files2003_Private : public Files_Private
{
    friend class Files2003;

public:
    ~Files2003_Private();
    QSharedPointer<Files_Private> clone() const;

protected:
    Files2003_Private(VcProjectDocument *parentProjectDocument);
    Files2003_Private(const Files2003_Private &filesPrivate);
    Files2003_Private& operator=(const Files2003_Private &filesPrivate);
};

class Files2005_Private : public Files_Private
{
    friend class Files2005;

public:
    ~Files2005_Private();
    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    QSharedPointer<Files_Private> clone() const;

    void addFolder(Folder::Ptr newFolder);
    void removeFolder(Folder::Ptr folder);
    QList<Folder::Ptr> folders() const;
    Folder::Ptr folder(const QString &folderName) const;

protected:
    Files2005_Private(VcProjectDocument *parentProjectDocument);
    Files2005_Private(const Files2005_Private &filesPrivate);
    Files2005_Private& operator=(const Files2005_Private &filesPrivate);

private:
    void processFile(const QDomNode &fileNode);
    void processFilter(const QDomNode &filterNode);
    void processFolder(const QDomNode &folderNode);

    QList<Folder::Ptr> m_folders;
};

class Files2008_Private : public Files_Private
{
    friend class Files2008;

public:
    ~Files2008_Private();
    QSharedPointer<Files_Private> clone() const;

protected:
    Files2008_Private(VcProjectDocument *parentProjectDocument);
    Files2008_Private(const Files2008_Private &filesPrivate);
    Files2008_Private& operator=(const Files2008_Private &filesPrivate);
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FILES_PRIVATE_H
