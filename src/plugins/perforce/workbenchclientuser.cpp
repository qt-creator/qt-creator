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

#include "workbenchclientuser.h"
#include "perforceoutputwindow.h"
#include "perforceplugin.h"

#include <coreplugin/filemanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QEventLoop>
#include <QtCore/QTemporaryFile>
#include <QtGui/QMessageBox>
#include <QtGui/QRadioButton>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

using namespace Perforce::Internal;

PromptDialog::PromptDialog(const QString &choice, const QString &text,
                           QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    m_ui.msgLabel->setText(text);

    const QChar closingParenthesis = QLatin1Char(')');
    const QStringList opts = choice.split(QString(closingParenthesis));
    int row = 0;
    int column = 0;
    QString opt;
    QRadioButton *rb = 0;
    for (int i=0; i<opts.count(); ++i) {
        opt = opts.at(i).trimmed();
        if (opt.isEmpty() || opt.startsWith(QLatin1String("Help")))
            continue;
        if (i == opts.count()-1)
            opt = QLatin1String("Default(") + opt.left(opt.length()-1);
        opt.append(QLatin1String(")"));
        rb = new QRadioButton(opt, this);
        rb->setChecked(true);
        if (column>0 && column%3==0)
            ++row;
        m_ui.gridLayout->addWidget(rb, row, column%3, 1, 1);
        ++column;

        const int j = opt.lastIndexOf(QLatin1Char('('));
        opt = opt.mid(j+1, opt.lastIndexOf(closingParenthesis)-j-1);
        m_optionsMap.insert(rb, opt);
    }
}

QString PromptDialog::input() const
{
    QMapIterator<QRadioButton*, QString> it(m_optionsMap);
    while (it.hasNext()) {
        it.next();
        if (it.key()->isChecked())
            return it.value();
    }
    return QString();
}

WorkbenchClientUser::WorkbenchClientUser(PerforceOutputWindow *out, PerforcePlugin *plugin)  :
    QObject(out),
    m_plugin(plugin),
    m_core(Core::ICore::instance()),
    m_currentEditorIface(0),
    m_userCancelled(false),
    m_mode(Submit),
    m_perforceOutputWindow(out),
    m_skipNextMsg(false),
    m_eventLoop(new QEventLoop(this))
{
    connect(m_core, SIGNAL(coreAboutToClose()),
        this, SLOT(cancelP4Command()));
}

WorkbenchClientUser::~WorkbenchClientUser()
{
}

void WorkbenchClientUser::setMode(WorkbenchClientUser::Mode mode)
{
    m_mode = mode;
}

void WorkbenchClientUser::cancelP4Command()
{
    m_userCancelled = true;
    m_eventLoop->quit();
}

void WorkbenchClientUser::Message(Error* err)
{
    StrBuf buf;
    err->Fmt(&buf);
    QString s = buf.Text();
    m_perforceOutputWindow->append(s);
    if (!m_skipNextMsg) {
        if (err->GetSeverity() == E_FAILED || err->GetSeverity() == E_FATAL) {
            if (!s.startsWith("Client side operation(s) failed."))
                m_errMsg.append(s);
        } else {
            m_msg.append(s);
        }
    }
    m_skipNextMsg = false;
}

void WorkbenchClientUser::displayErrorMsg(const QString &msg)
{
    if (msg.isEmpty())
        return;

    const QString title = tr("Perforce Error");
    switch (m_mode) {
    case Submit: {
        QMessageBox msgBox(QMessageBox::Critical, title,  msg, QMessageBox::Ok, m_core->mainWindow());
        msgBox.setDetailedText(m_msg);
        msgBox.exec();
    }
        break;
    default:
        QMessageBox::critical(m_core->mainWindow(), title, msg);
        break;
    }
    m_errMsg.clear();
}

void WorkbenchClientUser::OutputError(const char *errBuf)
{
    QString s(errBuf);
    s = s.trimmed();
    m_perforceOutputWindow->append(s);
    displayErrorMsg(s);
}

void WorkbenchClientUser::Finished()
{
    m_errMsg = m_errMsg.trimmed();
    displayErrorMsg(m_errMsg);
    m_msg.clear();
    m_currentEditorIface = 0;
    m_userCancelled = false;
    m_skipNextMsg = false;
}

bool WorkbenchClientUser::editorAboutToClose(Core::IEditor *editor)
{
    if (editor && editor == m_currentEditorIface) {
        if (m_mode == WorkbenchClientUser::Submit) {
            const QMessageBox::StandardButton answer =
                QMessageBox::question(m_core->mainWindow(),
                                      tr("Closing p4 Editor"),
                                      tr("Do you want to submit this change list?"),
                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes);
            if (answer == QMessageBox::Cancel)
                return false;
            if (answer == QMessageBox::No)
                m_userCancelled = true;
            m_core->fileManager()->blockFileChange(m_currentEditorIface->file());
            m_currentEditorIface->file()->save();
            m_core->fileManager()->unblockFileChange(m_currentEditorIface->file());
        }
        m_eventLoop->quit();
        m_currentEditorIface = 0;
    }
    return true;
}

void WorkbenchClientUser::Diff(FileSys *f1, FileSys *f2, int, char *, Error *err)
{
    if (!f1->IsTextual() || !f2->IsTextual())
        return;

    FileSys *file1 = File(FST_BINARY);
    file1->Set(f1->Name());

    FileSys *file2 = File(FST_BINARY);
    file2->Set(f2->Name());

    QTemporaryFile tmp;
    tmp.open();
    QString fileName = tmp.fileName();

    {
        ::Diff d;
        d.SetInput(file1, file2, DiffFlags(), err);
        if (!err->Test())
            d.SetOutput(fileName.toLatin1().constData(), err);
        if (!err->Test())
            d.DiffUnified();
        d.CloseOutput(err);
    }
    delete file1;
    delete file2;

    QString title = QString("diff %1").arg(f1->Name());
    m_currentEditorIface = m_core->editorManager()->newFile("Perforce Editor", &title, tmp.readAll());
    if (!m_currentEditorIface) {
        err->Set(E_FAILED, "p4 data could not be opened!");
        return;
    }
    m_userCancelled = false;
    m_eventLoop->exec();
    if (m_userCancelled)
        err->Set(E_FAILED, "");
}

void WorkbenchClientUser::Edit(FileSys *f, Error *err)
{
    QString fileName(f->Name());
    if (m_mode == Submit) {
        m_currentEditorIface = m_plugin->openPerforceSubmitEditor(fileName, QStringList());
    }
    else {
        m_currentEditorIface = m_core->editorManager()->openEditor(fileName);
        m_core->editorManager()->ensureEditorManagerVisible();
    }
    if (!m_currentEditorIface) {
        err->Set(E_FAILED, "p4 data could not be opened!");
        return;
    }
    m_userCancelled = false;
    m_eventLoop->exec();
    if (m_userCancelled)
        err->Set(E_FAILED, "");
}

void WorkbenchClientUser::Prompt(const StrPtr &msg, StrBuf &answer, int , Error *err)
{
    if (m_userCancelled) {
        err->Set(E_FATAL, "");
        return;
    }
    PromptDialog dia(msg.Text(), m_msg, qobject_cast<QWidget*>(m_core));
    dia.exec();
    answer = qstrdup(dia.input().toLatin1().constData());
    if (m_mode == WorkbenchClientUser::Resolve) {
        if (strcmp(answer.Text(), "e") == 0) {
            ;
        } else if (strcmp(answer.Text(), "d") == 0) {
            ;
        } else {
            m_msg.clear();
            m_skipNextMsg = true;
        }
    }
}

void WorkbenchClientUser::ErrorPause(char *msg, Error *)
{
    QMessageBox::warning(m_core->mainWindow(), tr("Perforce Error"), QString::fromUtf8(msg));
}
