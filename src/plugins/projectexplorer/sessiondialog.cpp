/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    m_switchToButton = buttons->addButton(tr("Switch To"), QDialogButtonBox::AcceptRole);
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


SessionDialog::SessionDialog(QWidget *parent)
    : QDialog(parent)
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
    QStringList sessions = SessionManager::sessions();
    foreach (const QString &session, sessions) {
        m_ui.sessionList->addItem(session);
        if (setDefaultSession && session == SessionManager::activeSession())
            m_ui.sessionList->setCurrentRow(m_ui.sessionList->count() - 1);
    }
}
void SessionDialog::markItems()
{
    for (int i = 0; i < m_ui.sessionList->count(); ++i) {
        QListWidgetItem *item = m_ui.sessionList->item(i);
        QFont f = item->font();
        QString session = item->data(Qt::DisplayRole).toString();
        if (SessionManager::isDefaultSession(session))
            f.setItalic(true);
        else
            f.setItalic(false);
        if (SessionManager::activeSession() == session && !SessionManager::isDefaultVirgin())
            f.setBold(true);
        else
            f.setBold(false);
        item->setFont(f);
    }
}

void SessionDialog::addSessionToUi(const QString &name, bool switchTo)
{
    m_ui.sessionList->clear();
    QStringList sessions = SessionManager::sessions();
    m_ui.sessionList->addItems(sessions);
    m_ui.sessionList->setCurrentRow(sessions.indexOf(name));
    markItems();
    if (switchTo)
        switchToSession();
}

void SessionDialog::updateActions()
{
    if (m_ui.sessionList->currentItem()) {
        bool isDefault = (m_ui.sessionList->currentItem()->text() == QLatin1String("default"));
        bool isActive = (m_ui.sessionList->currentItem()->text() == SessionManager::activeSession());
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
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), this);
    newSessionInputDialog.setWindowTitle(tr("New Session Name"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString sessionName = newSessionInputDialog.value();
        if (sessionName.isEmpty() || SessionManager::sessions().contains(sessionName))
            return;

        SessionManager::createSession(sessionName);
        addSessionToUi(sessionName, newSessionInputDialog.isSwitchToRequested());
    }
}

void SessionDialog::clone()
{
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), this);
    newSessionInputDialog.setValue(m_ui.sessionList->currentItem()->text());
    newSessionInputDialog.setWindowTitle(tr("New Session Name"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (SessionManager::cloneSession(m_ui.sessionList->currentItem()->text(), newSession))
            addSessionToUi(newSession, newSessionInputDialog.isSwitchToRequested());
    }
}

void SessionDialog::remove()
{
    const QString name = m_ui.sessionList->currentItem()->text();

    if (!SessionManager::confirmSessionDelete(name))
        return;
    SessionManager::deleteSession(name);
    m_ui.sessionList->clear();
    addItems(false);
    markItems();
}

void SessionDialog::rename()
{
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), this);
    newSessionInputDialog.setValue(m_ui.sessionList->currentItem()->text());
    newSessionInputDialog.setWindowTitle(tr("Rename Session"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        SessionManager::renameSession(m_ui.sessionList->currentItem()->text(), newSessionInputDialog.value());
        m_ui.sessionList->clear();
        addItems(false);
        markItems();
    }
}

void SessionDialog::switchToSession()
{
    QString session = m_ui.sessionList->currentItem()->text();
    SessionManager::loadSession(session);
    markItems();
    updateActions();
    reject();
}

} // namespace Internal
} // namespace ProjectExplorer
