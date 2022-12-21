// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sessiondialog.h"

#include "session.h"
#include "sessionview.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
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
    setObjectName("ProjectExplorer.SessionDialog");
    resize(550, 400);
    setWindowTitle(tr("Session Manager"));


    auto sessionView = new SessionView(this);
    sessionView->setObjectName("sessionView");
    sessionView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sessionView->setActivationMode(Utils::DoubleClickActivation);

    auto createNewButton = new QPushButton(tr("&New"));
    createNewButton->setObjectName("btCreateNew");

    m_openButton = new QPushButton(tr("&Open"));
    m_openButton->setObjectName("btOpen");
    m_renameButton = new QPushButton(tr("&Rename"));
    m_cloneButton = new QPushButton(tr("C&lone"));
    m_deleteButton = new QPushButton(tr("&Delete"));

    m_autoLoadCheckBox = new QCheckBox(tr("Restore last session on startup"));

    auto buttonBox = new QDialogButtonBox(this);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);

    m_openButton->setDefault(true);

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
                m_openButton,
                m_renameButton,
                m_cloneButton,
                m_deleteButton,
                st
            }
        },
        m_autoLoadCheckBox,
        hr,
        Row { whatsASessionLabel, buttonBox },
    }.attachTo(this);

    connect(createNewButton, &QAbstractButton::clicked,
            sessionView, &SessionView::createNewSession);
    connect(m_openButton, &QAbstractButton::clicked,
            sessionView, &SessionView::switchToCurrentSession);
    connect(m_renameButton, &QAbstractButton::clicked,
            sessionView, &SessionView::renameCurrentSession);
    connect(m_cloneButton, &QAbstractButton::clicked,
            sessionView, &SessionView::cloneCurrentSession);
    connect(m_deleteButton, &QAbstractButton::clicked,
            sessionView, &SessionView::deleteSelectedSessions);
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
        m_openButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_cloneButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        return;
    }
    const bool defaultIsSelected = sessions.contains("default");
    const bool activeIsSelected = Utils::anyOf(sessions, [](const QString &session) {
        return session == SessionManager::activeSession();
    });
    m_openButton->setEnabled(sessions.size() == 1);
    m_renameButton->setEnabled(sessions.size() == 1 && !defaultIsSelected);
    m_cloneButton->setEnabled(sessions.size() == 1);
    m_deleteButton->setEnabled(!defaultIsSelected && !activeIsSelected);
}

} // ProjectExplorer::Internal
