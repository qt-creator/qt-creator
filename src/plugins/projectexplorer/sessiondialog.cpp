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

#include "sessiondialog.h"
#include "session.h"

#include <QtGui/QInputDialog>
#include <QtGui/QValidator>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {
namespace Internal {

class SessionValidator : public QValidator
{
public:
    SessionValidator(QObject *parent, QStringList sessions);
    void fixup(QString & input) const;
    QValidator::State validate(QString & input, int & pos) const;
private:
    QStringList m_sessions;
};

SessionValidator::SessionValidator(QObject *parent, QStringList sessions)
    : QValidator(parent), m_sessions(sessions)
{
}

QValidator::State SessionValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    if (m_sessions.contains(input))
        return QValidator::Intermediate;
    else
        return QValidator::Acceptable;
}

void SessionValidator::fixup(QString &input) const
{
    int i = 2;
    QString copy;
    do {
        copy = input + QString(" (%1)").arg(i);
        ++i;
    } while (m_sessions.contains(copy));
    input = copy;
}

class NewSessionInputDialog : public QDialog
{
public:
    NewSessionInputDialog(QStringList sessions);
    QString value();
private:
    QLineEdit *m_newSessionLineEdit;
};

NewSessionInputDialog::NewSessionInputDialog(QStringList sessions)
{
    setWindowTitle("New session name");
    QVBoxLayout *hlayout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Enter the name of the new session:", this);
    hlayout->addWidget(label);
    m_newSessionLineEdit = new QLineEdit(this);
    m_newSessionLineEdit->setValidator(new SessionValidator(this, sessions));
    hlayout->addWidget(m_newSessionLineEdit);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    hlayout->addWidget(buttons);
    setLayout(hlayout);
}

QString NewSessionInputDialog::value()
{
    return m_newSessionLineEdit->text();
}

SessionDialog::SessionDialog(SessionManager *sessionManager, const QString &lastSession, bool startup)
    : m_sessionManager(sessionManager), m_startup(startup)
{
    m_ui.setupUi(this);

    connect(m_ui.btCreateNew, SIGNAL(clicked()),
            this, SLOT(createNew()));

    connect(m_ui.btClone, SIGNAL(clicked()),
            this, SLOT(clone()));
    connect(m_ui.btDelete, SIGNAL(clicked()),
            this, SLOT(remove()));

    connect(m_ui.sessionList, SIGNAL(itemDoubleClicked ( QListWidgetItem *)),
            this, SLOT(accept()));

    connect(m_ui.sessionList, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(updateActions()));

    QStringList sessions = sessionManager->sessions();
    foreach (const QString &session, sessions) {
        m_ui.sessionList->addItem(session);
        if (session == lastSession)
            m_ui.sessionList->setCurrentRow(m_ui.sessionList->count() - 1);
    }
}

void SessionDialog::updateActions()
{
    bool enableDelete = false;

    if (m_ui.sessionList->currentItem())
        enableDelete = (m_ui.sessionList->currentItem()->text() != m_sessionManager->activeSession()
                        && m_ui.sessionList->currentItem()->text() != "default");
    m_ui.btDelete->setEnabled(enableDelete);
}

void SessionDialog::accept()
{
    if (m_ui.sessionList->currentItem()) {
        m_sessionManager->loadSession(m_ui.sessionList->currentItem()->text());
    }
    QDialog::accept();
}

void SessionDialog::reject()
{
    // clear list
    QDialog::reject();
}

void SessionDialog::createNew()
{
    NewSessionInputDialog newSessionInputDialog(m_sessionManager->sessions());
    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || m_sessionManager->sessions().contains(newSession))
            return;

        m_sessionManager->createSession(newSession);
        m_ui.sessionList->addItem(newSession);
        m_ui.sessionList->setCurrentRow(m_ui.sessionList->count() - 1);
    }
}

void SessionDialog::clone()
{
    NewSessionInputDialog newSessionInputDialog(m_sessionManager->sessions());
    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (m_sessionManager->cloneSession(m_ui.sessionList->currentItem()->text(), newSession))
            m_ui.sessionList->addItem(newSession);
    }
}

void SessionDialog::remove()
{
    m_sessionManager->deleteSession(m_ui.sessionList->currentItem()->text());
    m_ui.sessionList->clear();
    m_ui.sessionList->addItems(m_sessionManager->sessions());
}

} // namespace Internal
} // namespace ProjectExplorer
