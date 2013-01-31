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

#include "sessiondialog.h"
#include "session.h"

#include <QInputDialog>
#include <QValidator>

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

    if (input.contains(QLatin1Char('/'))
            || input.contains(QLatin1Char(':'))
            || input.contains(QLatin1Char('\\'))
            || input.contains(QLatin1Char('?'))
            || input.contains(QLatin1Char('*')))
        return QValidator::Invalid;

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
        copy = input + QLatin1String(" (") + QString::number(i) + QLatin1Char(')');
        ++i;
    } while (m_sessions.contains(copy));
    input = copy;
}

SessionNameInputDialog::SessionNameInputDialog(const QStringList &sessions, QWidget *parent)
    : QDialog(parent), m_usedSwitchTo(false)
{
    QVBoxLayout *hlayout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Enter the name of the session:"), this);
    hlayout->addWidget(label);
    m_newSessionLineEdit = new QLineEdit(this);
    m_newSessionLineEdit->setValidator(new SessionValidator(this, sessions));
    hlayout->addWidget(m_newSessionLineEdit);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_switchToButton = buttons->addButton(tr("Switch to"), QDialogButtonBox::AcceptRole);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
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

void SessionNameInputDialog::clicked(QAbstractButton *button)
{
    if (button == m_switchToButton)
        m_usedSwitchTo = true;
}

bool SessionNameInputDialog::isSwitchToRequested() const
{
    return m_usedSwitchTo;
}


SessionDialog::SessionDialog(SessionManager *sessionManager, QWidget *parent)
    : QDialog(parent), m_sessionManager(sessionManager)
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

    connect(m_ui.sessionList, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(switchToSession()));

    connect(m_ui.sessionList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(updateActions()));

    m_ui.whatsASessionLabel->setOpenExternalLinks(true);
    addItems(true);
    markItems();
}

void SessionDialog::setAutoLoadSession(bool check)
{
    m_ui.autoLoadCheckBox->setChecked(check ? Qt::Checked : Qt::Unchecked);
}

bool SessionDialog::autoLoadSession() const
{
    return m_ui.autoLoadCheckBox->checkState() == Qt::Checked;
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
    for (int i = 0; i < m_ui.sessionList->count(); ++i) {
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
    if (m_ui.sessionList->currentItem()) {
        bool isDefault = (m_ui.sessionList->currentItem()->text() == QLatin1String("default"));
        bool isActive = (m_ui.sessionList->currentItem()->text() == m_sessionManager->activeSession());
        m_ui.btDelete->setEnabled(!isActive && !isDefault);
        m_ui.btRename->setEnabled(!isDefault);
        m_ui.btClone->setEnabled(true);
        m_ui.btSwitch->setEnabled(true);
    } else {
        m_ui.btDelete->setEnabled(false);
        m_ui.btRename->setEnabled(false);
        m_ui.btClone->setEnabled(false);
        m_ui.btSwitch->setEnabled(false);
    }
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
        markItems();
        if (newSessionInputDialog.isSwitchToRequested())
            switchToSession();
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
            markItems();
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
    QString session = m_ui.sessionList->currentItem()->text();
    m_sessionManager->loadSession(session);
    markItems();
    updateActions();
    reject();
}

} // namespace Internal
} // namespace ProjectExplorer
