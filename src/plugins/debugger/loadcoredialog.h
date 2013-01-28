/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

    int exec();

    QString localExecutableFile() const;
    QString localCoreFile() const;
    QString remoteCoreFile() const;
    QString overrideStartScript() const;
    bool useLocalCoreFile() const;
    bool forcesLocalCoreFile() const;
    bool isLocalKit() const;

    // For persistance.
    ProjectExplorer::Kit *kit() const;
    void setLocalExecutableFile(const QString &executable);
    void setLocalCoreFile(const QString &core);
    void setRemoteCoreFile(const QString &core);
    void setOverrideStartScript(const QString &scriptName);
    void setKitId(const Core::Id &id);
    void setForceLocalCoreFile(bool on);

private slots:
    void changed();
    void selectRemoteCoreFile();

private:
    AttachCoreDialogPrivate *d;
};


} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_ATTACHCOREDIALOG_H
