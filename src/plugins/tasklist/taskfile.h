/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef TASKFILE_H
#define TASKFILE_H

#include <coreplugin/idocument.h>

namespace ProjectExplorer {
class Project;
} // namespace ProjectExplorer

namespace TaskList {
namespace Internal {

class TaskFile : public Core::IDocument
{
public:
    TaskFile(QObject *parent);
    ~TaskFile();

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    bool open(QString *errorString, const QString &fileName);

    ProjectExplorer::Project *context() const;
    void setContext(ProjectExplorer::Project *context);

private:
    QString m_fileName;
    ProjectExplorer::Project *m_context;
};

} // namespace Internal
} // namespace TaskList

#endif // TASKFILE_H
