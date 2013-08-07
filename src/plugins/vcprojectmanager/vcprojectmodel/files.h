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
#ifndef VCPROJECTMANAGER_INTERNAL_FILES_H
#define VCPROJECTMANAGER_INTERNAL_FILES_H

#include "ivcprojectnodemodel.h"

#include "file.h"
#include "filter.h"
#include "folder.h"

namespace VcProjectManager {
namespace Internal {

class Files : public IVcProjectXMLNode
{
public:
    typedef QSharedPointer<Files>   Ptr;

    ~Files();
    void processNode(const QDomNode &node);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    virtual bool isEmpty() const;

    /*!
     * Implement in order to support creating a clone of a Files instance.
     * \return A shared pointer to newly created Files instance.
     */
    virtual Files* clone() const = 0;

    void addFilter(Filter::Ptr newFilter);
    void removeFilter(Filter::Ptr filter);
    void removeFilter(const QString &filterName);
    QList<Filter::Ptr> filters() const;
    Filter::Ptr filter(const QString &filterName) const;

    void addFile(File::Ptr file);
    void removeFile(File::Ptr file);
    QList<File::Ptr> files() const;
    File::Ptr file(const QString &relativePath) const;
    virtual bool fileExists(const QString &relativeFilePath) const;

    virtual void allProjectFiles(QStringList &sl) const;

protected:
    Files(VcProjectDocument *parentProject);
    Files(const Files &files);
    Files& operator=(const Files &files);

    virtual void processFile(const QDomNode &fileNode);
    virtual void processFilter(const QDomNode &filterNode);

    QList<Filter::Ptr> m_filters;
    QList<File::Ptr> m_files;
    VcProjectDocument *m_parentProject;
};

class Files2003 : public Files
{
public:
    typedef QSharedPointer<Files2003>   Ptr;

    Files2003(VcProjectDocument *parentProjectDocument);
    Files2003(const Files2003 &files);
    Files2003& operator=(const Files2003 &files);
    ~Files2003();

    Files* clone() const;
};

class Files2005 : public Files
{
public:
    typedef QSharedPointer<Files2005>   Ptr;

    Files2005(VcProjectDocument *parentProjectDocument);
    Files2005(const Files2005 &files);
    Files2005& operator=(const Files2005 &files);
    ~Files2005();

    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    bool isEmpty() const;
    Files* clone() const;
    bool fileExists(const QString &relativeFilePath) const;

    void addFolder(Folder::Ptr newFolder);
    void removeFolder(Folder::Ptr folder);
    void removeFolder(const QString &folderName);
    QList<Folder::Ptr> folders() const;
    Folder::Ptr folder(const QString &folderName) const;

    void allProjectFiles(QStringList &sl) const;

protected:
    void processFile(const QDomNode &fileNode);
    void processFilter(const QDomNode &filterNode);
    void processFolder(const QDomNode &folderNode);

    QList<Folder::Ptr> m_folders;
};

class Files2008 : public Files
{
public:
    typedef QSharedPointer<Files2008>   Ptr;

    Files2008(VcProjectDocument *parentProjectDocument);
    Files2008(const Files2008 &files);
    Files2008& operator=(const Files2008 &files);
    ~Files2008();

    Files* clone() const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FILES_H
