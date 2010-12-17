/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SUBMITEDITORFILE_H
#define SUBMITEDITORFILE_H

#include <coreplugin/ifile.h>

namespace VCSBase {
namespace Internal {

// A non-saveable IFile for submit editor files.
class SubmitEditorFile : public Core::IFile
{
    Q_OBJECT
public:
    explicit SubmitEditorFile(const QString &mimeType,
                              QObject *parent = 0);

    QString fileName() const { return m_fileName; }
    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }

    bool isModified() const { return m_modified; }
    virtual QString mimeType() const;
    bool isReadOnly() const { return false; }
    bool isSaveAsAllowed() const { return false; }
    bool save(const QString &fileName);
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    void setFileName(const QString name);
    void setModified(bool modified = true);

signals:
    void saveMe(const QString &fileName);

private:
    const QString m_mimeType;
    bool m_modified;
    QString m_fileName;
};


} // namespace Internal
} // namespace VCSBase

#endif // SUBMITEDITORFILE_H
