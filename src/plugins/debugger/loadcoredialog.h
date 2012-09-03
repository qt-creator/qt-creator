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

#ifndef DEBUGGER_ATTACHCOREDIALOG_H
#define DEBUGGER_ATTACHCOREDIALOG_H

#include <QDialog>

namespace Core { class Id; }
namespace ProjectExplorer { class Kit; }

namespace Debugger {
namespace Internal {

class AttachCoreDialogPrivate;

class AttachCoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachCoreDialog(QWidget *parent);
    ~AttachCoreDialog();

    QString localExecutableFile() const;
    QString localCoreFile() const;
    QString remoteCoreFile() const;
    QString overrideStartScript() const;
    bool isLocal() const;

    // For persistance.
    ProjectExplorer::Kit *kit() const;
    void setLocalExecutableFile(const QString &executable);
    void setLocalCoreFile(const QString &core);
    void setRemoteCoreFile(const QString &core);
    void setOverrideStartScript(const QString &scriptName);
    void setKitId(const Core::Id &id);

private slots:
    void changed();
    void selectRemoteCoreFile();

private:
    AttachCoreDialogPrivate *d;
};


} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_ATTACHCOREDIALOG_H
