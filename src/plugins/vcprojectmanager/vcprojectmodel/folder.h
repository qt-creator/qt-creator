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
#ifndef VCPROJECTMANAGER_INTERNAL_FOLDER_H
#define VCPROJECTMANAGER_INTERNAL_FOLDER_H

#include "file.h"
#include "filter.h"
#include "../interfaces/ifilecontainer.h"

namespace VcProjectManager {
namespace Internal {

class Folder : public IFileContainer
{
public:
    Folder(VcProjectDocument *parentProjectDoc);
    Folder(const Folder &folder);
    Folder& operator=(const Folder &folder);
    ~Folder();

    QString containerType() const;

    void processNode(const QDomNode &node);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    void addFile(IFile *file);
    void removeFile(IFile *file);
    void removeFile(const QString &relativeFilePath);
    QList<IFile *> files() const;
    IFile* file(const QString &relativeFilePath) const;
    IFile *file(int index) const;
    int fileCount() const;
    void addFileContainer(IFileContainer *fileContainer);
    int childCount() const;
    IFileContainer* fileContainer(int index) const;
    void removeFileContainer(IFileContainer *fileContainer);
    IAttributeContainer *attributeContainer() const;
    bool fileExists(const QString &relativeFilePath) const;

    QString name() const;
    void setName(const QString &name);

    void allFiles(QStringList &sl) const;
    IFileContainer* clone() const;

private:
    void processFile(const QDomNode &fileNode);
    void processFilter(const QDomNode &filterNode);
    void processFolder(const QDomNode &folderNode);
    void processNodeAttributes(const QDomElement &element);

    QList<IFile *> m_files;
    QList<IFileContainer *> m_fileContainers;

    QString m_name; // required
    VcProjectDocument *m_parentProjectDoc;
    IAttributeContainer *m_attributeContainer;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FOLDER_H
