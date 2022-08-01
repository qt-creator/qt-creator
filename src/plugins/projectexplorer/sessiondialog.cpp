/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sessiondialog.h"

#include "session.h"
#include "sessionview.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QValidator>

namespace ProjectExplorer::Internal {

class SessionValidator : public QValidator
{
public:
    SessionValidator(QObject *parent, const QStringList &sessions);
    void fixup(QString & input) const override;
    QValidator::State validate(QString & input, int & pos) const override;
private:
    QStringList m_sessions;
};

SessionValidator::SessionValidator(QObject *parent, const QStringList &sessions)
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

SessionNameInputDialog::SessionNameInputDialog(QWidget *parent)
    : QDialog(parent)
{
    m_newSessionLineEdit = new QLineEdit(this);
    m_newSessionLineEdit->setValidator(new SessionValidator(this, SessionManager::sessions()));

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_switchToButton = new QPushButton;
    buttons->addButton(m_switchToButton, QDialogButtonBox::AcceptRole);
    connect(m_switchToButton, &QPushButton::clicked, this, [this] {
        m_usedSwitchTo = true;
    });

    using namespace Utils::Layouting;
    Column {
        tr("Enter the name of the session:"),
        m_newSessionLineEdit,
        buttons,
    }.attachTo(this);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SessionNameInputDialog::setActionText(const QString &actionText, const QString &openActionText)
{
    m_okButton->setText(actionText);
    m_switchToButton->setText(openActionText);
}

void SessionNameInputDialog::setValue(const QString &value)
{
    m_newSessionLineEdit->setText(value);
}

QString SessionNameInputDialog::value() const
{
    return m_newSessionLineEdit->text();
}

bool SessionNameInputDialog::isSwitchToRequested() const
{
    return m_usedSwitchTo;
}

SessionDialog::SessionDialog(QWidget *parent) : QDialog(parent)
{
    resize(550, 400);
    setWindowTitle(tr("Session Manager"));


    auto sessionView = new SessionView(this);
    sessionView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sessionView->setActivationMode(Utils::DoubleClickActivation);

    auto createNewButton = new QPushButton(tr("&New"));

    m_renameButton = new QPushButton(tr("&Rename"));
    m_cloneButton = new QPushButton(tr("C&lone"));
    m_deleteButton = new QPushButton(tr("&Delete"));
    m_switchButton = new QPushButton(tr("&Switch To"));

    m_autoLoadCheckBox = new QCheckBox(tr("Restore last session on startup"));

    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);

    m_switchButton->setDefault(true);

    // FIXME: Simplify translator's work.
    auto whatsASessionLabel = new QLabel(
        tr("<a href=\"qthelp://org.qt-project.qtcreator/doc/creator-project-managing-sessions.html\">"
           "What is a Session?</a>"));
    whatsASessionLabel->setOpenExternalLinks(true);

    using namespace Utils::Layouting;

    Column {
        Row {
            sessionView,
            Column {
                createNewButton,
                m_renameButton,
                m_cloneButton,
                m_deleteButton,
                m_switchButton,
                st
            }
        },
        m_autoLoadCheckBox,
        line,
        Row { whatsASessionLabel, buttonBox },
    }.attachTo(this);

    connect(createNewButton, &QAbstractButton::clicked,
            sessionView, &SessionView::createNewSession);
    connect(m_cloneButton, &QAbstractButton::clicked,
            sessionView, &SessionView::cloneCurrentSession);
    connect(m_deleteButton, &QAbstractButton::clicked,
            sessionView, &SessionView::deleteSelectedSessions);
    connect(m_switchButton, &QAbstractButton::clicked,
            sessionView, &SessionView::switchToCurrentSession);
    connect(m_renameButton, &QAbstractButton::clicked,
            sessionView, &SessionView::renameCurrentSession);
    connect(sessionView, &SessionView::sessionActivated,
            sessionView, &SessionView::switchToCurrentSession);

    connect(sessionView, &SessionView::sessionsSelected,
            this, &SessionDialog::updateActions);
    connect(sessionView, &SessionView::sessionSwitched,
            this, &QDialog::reject);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

void SessionDialog::setAutoLoadSession(bool check)
{
    m_autoLoadCheckBox->setChecked(check);
}

bool SessionDialog::autoLoadSession() const
{
    return m_autoLoadCheckBox->checkState() == Qt::Checked;
}

void SessionDialog::updateActions(const QStringList &sessions)
{
    if (sessions.isEmpty()) {
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_cloneButton->setEnabled(false);
        m_switchButton->setEnabled(false);
        return;
    }
    const bool defaultIsSelected = sessions.contains("default");
    const bool activeIsSelected = Utils::anyOf(sessions, [](const QString &session) {
        return session == SessionManager::activeSession();
    });
    m_deleteButton->setEnabled(!defaultIsSelected && !activeIsSelected);
    m_renameButton->setEnabled(sessions.size() == 1 && !defaultIsSelected);
    m_cloneButton->setEnabled(sessions.size() == 1);
    m_switchButton->setEnabled(sessions.size() == 1);
}

} // ProjectExplorer::Internal
