/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
    PromptDialog(const QString &choice, const QString &text,
        QWidget *parent = 0);
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
    Core::ICore *m_coreIFace;
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
