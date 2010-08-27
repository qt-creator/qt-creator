/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TASKFILE_H
#define TASKFILE_H

#include <coreplugin/ifile.h>

namespace ProjectExplorer {
class Project;
} // namespace ProjectExplorer

namespace TaskList {
namespace Internal {

class TaskFile : public Core::IFile
{
public:
    TaskFile(QObject *parent);
    ~TaskFile();

    bool save(const QString &fileName = QString());
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    bool open(const QString &fileName);

    ProjectExplorer::Project *context() const;
    void setContext(ProjectExplorer::Project *context);

private:
    QString m_fileName;
    ProjectExplorer::Project *m_context;
};

} // namespace Internal
} // namespace TaskList

#endif // TASKFILE_H
