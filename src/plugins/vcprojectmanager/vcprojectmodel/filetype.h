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
#ifndef VCPROJECTMANAGER_INTERNAL_FILETYPE_H
#define VCPROJECTMANAGER_INTERNAL_FILETYPE_H

#include <projectexplorer/projectnodes.h>

#include "fileconfiguration.h"
#include "vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class File;
class VcProjectDocument;

typedef QSharedPointer<File> Ptr;

class FileType
{
    friend class File;

public:
    typedef QSharedPointer<FileType>    Ptr;

    ~FileType();

    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    ProjectExplorer::FileType fileType() const;
    FileType::Ptr clone() const;

    void addFile(QSharedPointer<File> file);
    void removeFile(QSharedPointer<File> file);
    void addFileConfiguration(FileConfiguration::Ptr fileConfig);
    void removeFileConfiguration(FileConfiguration::Ptr fileConfig);

    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);

    QString relativePath() const;
    void setRelativePath(const QString &relativePath);

    QString canonicalPath() const;

private:
    FileType(VcProjectDocument *parentProjectDoc);
    FileType(const FileType &fileType);
    FileType& operator=(const FileType &fileType);
    void processNodeAttributes(const QDomElement &element);
    void processFileConfiguration(const QDomNode &fileConfigNode);
    void processFile(const QDomNode &fileNode);

    QString m_relativePath; // required
    QList<QSharedPointer<File> > m_files;
    QList<FileConfiguration::Ptr > m_fileConfigurations;
    QHash<QString, QString> m_anyAttribute;
    VcProjectDocument *m_parentProjectDoc;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_FILETYPE_H
