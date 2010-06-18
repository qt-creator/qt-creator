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
    Q_UNUSED(pos)
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


class SessionNameInputDialog : public QDialog
{
    Q_OBJECT
public:
    SessionNameInputDialog(const QStringList &sessions, QWidget *parent = 0);

    void setValue(const QString &value);
    QString value() const;

private:
    QLineEdit *m_newSessionLineEdit;
};

SessionNameInputDialog::SessionNameInputDialog(const QStringList &sessions, QWidget *parent)
    : QDialog(parent)
{
    QVBoxLayout *hlayout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Enter the name of the session:"), this);
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

void SessionNameInputDialog::setValue(const QString &value)
{
    m_newSessionLineEdit->setText(value);
}

QString SessionNameInputDialog::value() const
{
    return m_newSessionLineEdit->text();
}


SessionDialog::SessionDialog(SessionManager *sessionManager)
    : m_sessionManager(sessionManager)
{
    m_ui.setupUi(this);

    connect(m_ui.btCreateNew, SIGNAL(clicked()),
            this, SLOT(createNew()));
    connect(m_ui.btClone, SIGNAL(clicked()),
            this, SLOT(clone()));
    connect(m_ui.btDelete, SIGNAL(clicked()),
            this, SLOT(remove()));

    connect(m_ui.btSwitch, SIGNAL(clicked()), this, SLOT(switchToSession()));
    connect(m_ui.btRename, SIGNAL(clicked()), this, SLOT(rename()));

    connect(m_ui.sessionList, SIGNAL(itemDoubleClicked (QListWidgetItem *)),
            this, SLOT(switchToSession()));

    connect(m_ui.sessionList, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(updateActions()));

    m_ui.whatsASessionLabel->setOpenExternalLinks(true);
    addItems(true);
    markItems();
}

void SessionDialog::addItems(bool setDefaultSession)
{
    QStringList sessions = m_sessionManager->sessions();
    foreach (const QString &session, sessions) {
        m_ui.sessionList->addItem(session);
        if (setDefaultSession && session == m_sessionManager->activeSession())
            m_ui.sessionList->setCurrentRow(m_ui.sessionList->count() - 1);
    }
}
void SessionDialog::markItems()
{
    for(int i = 0; i < m_ui.sessionList->count(); ++i) {
        QListWidgetItem *item = m_ui.sessionList->item(i);
        QFont f = item->font();
        QString session = item->data(Qt::DisplayRole).toString();
        if (m_sessionManager->isDefaultSession(session))
            f.setItalic(true);
        else
            f.setItalic(false);
        if (m_sessionManager->activeSession() == session && !m_sessionManager->isDefaultVirgin())
            f.setBold(true);
        else
            f.setBold(false);
        item->setFont(f);
    }
}

void SessionDialog::updateActions()
{
    bool isDefault = false;
    bool isActive = false;

    if (m_ui.sessionList->currentItem()) {
        isDefault = (m_ui.sessionList->currentItem()->text() == QLatin1String("default"));
        isActive = (m_ui.sessionList->currentItem()->text() == m_sessionManager->activeSession());
    }

    m_ui.btDelete->setDisabled(isActive || isDefault);
    m_ui.btRename->setDisabled(isDefault);
}

void SessionDialog::createNew()
{
    SessionNameInputDialog newSessionInputDialog(m_sessionManager->sessions(), this);
    newSessionInputDialog.setWindowTitle(tr("New session name"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || m_sessionManager->sessions().contains(newSession))
            return;

        m_sessionManager->createSession(newSession);
        m_ui.sessionList->clear();
        QStringList sessions = m_sessionManager->sessions();
        m_ui.sessionList->addItems(sessions);
        m_ui.sessionList->setCurrentRow(sessions.indexOf(newSession));
    }
}

void SessionDialog::clone()
{
    SessionNameInputDialog newSessionInputDialog(m_sessionManager->sessions(), this);
    newSessionInputDialog.setValue(m_ui.sessionList->currentItem()->text());
    newSessionInputDialog.setWindowTitle(tr("New session name"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (m_sessionManager->cloneSession(m_ui.sessionList->currentItem()->text(), newSession)) {
            m_ui.sessionList->clear();
            QStringList sessions = m_sessionManager->sessions();
            m_ui.sessionList->addItems(sessions);
            m_ui.sessionList->setCurrentRow(sessions.indexOf(newSession));
        }
    }
}

void SessionDialog::remove()
{
    m_sessionManager->deleteSession(m_ui.sessionList->currentItem()->text());
    m_ui.sessionList->clear();
    addItems(false);
    markItems();
}

void SessionDialog::rename()
{
    SessionNameInputDialog newSessionInputDialog(m_sessionManager->sessions(), this);
    newSessionInputDialog.setValue(m_ui.sessionList->currentItem()->text());
    newSessionInputDialog.setWindowTitle(tr("Rename session"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        m_sessionManager->renameSession(m_ui.sessionList->currentItem()->text(), newSessionInputDialog.value());
        m_ui.sessionList->clear();
        addItems(false);
        markItems();
    }
}

void SessionDialog::switchToSession()
{
    if (m_ui.sessionList->currentItem()) {
        QString session = m_ui.sessionList->currentItem()->text();
        m_sessionManager->loadSession(session);
        markItems();
    }
    updateActions();
}

} // namespace Internal
} // namespace ProjectExplorer

#include "sessiondialog.moc"
