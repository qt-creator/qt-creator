/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef WORKBENCHCLIENTUSER_H
#define WORKBENCHCLIENTUSER_H

#include "p4.h"
#include "ui_promptdialog.h"

#include <coreplugin/icorelistener.h>

#include <QtCore/QObject>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QEventLoop;
QT_END_NAMESPACE

namespace Core {
class ICore;
class IEditor;
}

namespace Perforce {
namespace Internal {

class PerforceOutputWindow;
class PerforcePlugin;

class PromptDialog : public QDialog
{
public:
    PromptDialog(const QString &choice, const QString &text, QWidget *parent = 0);
    QString input() const;

private:
    Ui::PromptDialog m_ui;
    QMap<QRadioButton*, QString> m_optionsMap;
};

class WorkbenchClientUser : public QObject, public ClientUser
{
    Q_OBJECT

public:
    enum Mode {Submit, Resolve};
    WorkbenchClientUser(PerforceOutputWindow *out, PerforcePlugin *plugin);
    ~WorkbenchClientUser();
    void setMode(WorkbenchClientUser::Mode mode);

    void Message(Error* err);
    void OutputError(const char *errBuf);
    void Finished();
    void Diff(FileSys *f1, FileSys *f2, int, char *, Error *err);
    void Edit( FileSys *f, Error *err);
    void Prompt(const StrPtr &msg, StrBuf &answer, int , Error *err);
    void ErrorPause(char *msg, Error *);
    bool editorAboutToClose(Core::IEditor *editor);

private slots:
    void cancelP4Command();

private:
    void displayErrorMsg(const QString &msg);

    PerforcePlugin *m_plugin;
    Core::ICore *m_core;
    Core::IEditor *m_currentEditorIface;
    bool m_userCancelled;
    Mode m_mode;
    PerforceOutputWindow *m_perforceOutputWindow;
    QString m_msg;
    QString m_errMsg;
    bool m_skipNextMsg;
    QEventLoop *m_eventLoop;
};

} // namespace Perforce
} // namespace Internal

#endif // WORKBENCHCLIENTUSER_H
