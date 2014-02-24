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
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H

#include <coreplugin/idocument.h>

#include "vcprojectmodel/vcprojectdocument_constants.h"

#include <QString>

namespace VcProjectManager {
namespace Internal {

class VcDocProjectNode;
class VcDocumentModel;

class VcProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    VcProjectFile(const QString &filePath, VcDocConstants::DocumentVersion docVersion);
    ~VcProjectFile();

    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    QString filePath();
    QString path();

    VcDocProjectNode *createVcDocNode() const;
    void reloadVcDoc();
    VcDocumentModel *documentModel() const;
private:
    QString m_filePath;
    QString m_path;
    VcDocumentModel *m_documentModel;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTFILE_H
