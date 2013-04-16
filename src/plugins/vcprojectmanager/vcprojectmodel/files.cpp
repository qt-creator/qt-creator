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
#include "files.h"

namespace VcProjectManager {
namespace Internal {

Files::~Files()
{
}

void Files::processNode(const QDomNode &node)
{
    m_private->processNode(node);
}

void Files::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *Files::createSettingsWidget()
{
    return 0;
}

QDomNode Files::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_private->toXMLDomNode(domXMLDocument);
}

bool Files::isEmpty() const
{
    return m_private->files().isEmpty() && m_private->filters().isEmpty();
}

void Files::addFilter(Filter::Ptr newFilter)
{
    m_private->addFilter(newFilter);
}

void Files::removeFilter(Filter::Ptr filter)
{
    m_private->removeFilter(filter);
}

QList<Filter::Ptr > Files::filters() const
{
    return m_private->filters();
}

Filter::Ptr Files::filter(const QString &filterName) const
{
    return m_private->filter(filterName);
}

void Files::addFile(File::Ptr file)
{
    m_private->addFile(file);
}

void Files::removeFile(File::Ptr file)
{
    m_private->removeFile(file);
}

QList<File::Ptr > Files::files() const
{
    return m_private->files();
}

File::Ptr Files::file(const QString &relativePath) const
{
    return m_private->file(relativePath);
}

void Files::allProjectFiles(QStringList &sl) const
{
    QList<Filter::Ptr > filters = m_private->filters();
    QList<File::Ptr > files = m_private->files();

    foreach (Filter::Ptr filter, filters)
        filter->allFiles(sl);

    foreach (File::Ptr file, files)
        sl.append(file->canonicalPath());
}

Files::Files(Files_Private *pr)
    : m_private(pr)
{
}

Files::Files(const Files &files)
{
    m_private = files.m_private->clone();
}

Files &Files::operator=(const Files &files)
{
    if (this != &files) {
        m_private = files.m_private->clone();
    }
    return *this;
}


Files2003::Files2003(VcProjectDocument *parentProjectDocument)
    : Files(new Files2003_Private(parentProjectDocument))
{
}

Files2003::Files2003(const Files2003 &files)
    : Files(files)
{
}

Files2003 &Files2003::operator =(const Files2003 &files)
{
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
    : Files(new Files2005_Private(parentProjectDocument))
{
}

Files2005::Files2005(const Files2005 &files)
    : Files(files)
{
}

Files2005 &Files2005::operator=(const Files2005 &files)
{
    Files::operator =(files);
    return *this;
}

Files2005::~Files2005()
{
}

bool Files2005::isEmpty() const
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    return Files::isEmpty() && files_p->folders().isEmpty();
}

Files *Files2005::clone() const
{
    return new Files2005(*this);
}

void Files2005::addFolder(Folder::Ptr newFolder)
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    files_p->addFolder(newFolder);
}

void Files2005::removeFolder(Folder::Ptr folder)
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    files_p->removeFolder(folder);
}

QList<Folder::Ptr > Files2005::folders() const
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    return files_p->folders();
}

Folder::Ptr Files2005::folder(const QString &folderName) const
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    return files_p->folder(folderName);
}

void Files2005::allProjectFiles(QStringList &sl) const
{
    QSharedPointer<Files2005_Private> files_p = m_private.staticCast<Files2005_Private>();
    QList<Folder::Ptr > folders = files_p->folders();
    QList<Filter::Ptr > filters = m_private->filters();
    QList<File::Ptr > files = m_private->files();

    foreach (Filter::Ptr filter, filters)
        filter->allFiles(sl);

    foreach (Folder::Ptr filter, folders)
        filter->allFiles(sl);

    foreach (File::Ptr file, files)
        sl.append(file->canonicalPath());
}


Files2008::Files2008(VcProjectDocument *parentProjectDocument)
    : Files(new Files2008_Private(parentProjectDocument))
{
}

Files2008::Files2008(const Files2008 &files)
    : Files(files)
{
}

Files2008 &Files2008::operator=(const Files2008 &files)
{
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
