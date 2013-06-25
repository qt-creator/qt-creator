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

#include "commitdata.h"
#include "gitsubmiteditorwidget.h"
#include "githighlighters.h"
#include "logchangedialog.h"

#include <QRegExpValidator>
#include <QTextEdit>

#include <QDir>
#include <QGroupBox>
#include <QRegExp>
#include <QVBoxLayout>

namespace Git {
namespace Internal {

// ------------------
GitSubmitEditorWidget::GitSubmitEditorWidget(QWidget *parent) :
    VcsBase::SubmitEditorWidget(parent),
    m_gitSubmitPanel(new QWidget),
    m_logChangeWidget(0),
    m_hasUnmerged(false),
    m_isInitialized(false)
{
    m_gitSubmitPanelUi.setupUi(m_gitSubmitPanel);
    new GitSubmitHighlighter(descriptionEdit());

    m_emailValidator = new QRegExpValidator(QRegExp(QLatin1String("[^@ ]+@[^@ ]+\\.[a-zA-Z]+")), this);

    connect(m_gitSubmitPanelUi.authorLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
    connect(m_gitSubmitPanelUi.emailLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(authorInformationChanged()));
}

void GitSubmitEditorWidget::setPanelInfo(const GitSubmitEditorPanelInfo &info)
{
    m_gitSubmitPanelUi.repositoryLabel->setText(QDir::toNativeSeparators(info.repository));
    if (info.branch.contains(QLatin1String("(no branch)")))
        m_gitSubmitPanelUi.branchLabel->setText(QString::fromLatin1("<span style=\"color:red\">%1</span>")
                                                .arg(tr("Detached HEAD")));
    else
        m_gitSubmitPanelUi.branchLabel->setText(info.branch);
}

QString GitSubmitEditorWidget::amendSHA1() const
{
    return m_logChangeWidget ? m_logChangeWidget->commit() : QString();
}

void GitSubmitEditorWidget::setHasUnmerged(bool e)
{
    m_hasUnmerged = e;
}

void GitSubmitEditorWidget::initialize(CommitType commitType, const QString &repository)
{
    if (m_isInitialized)
        return;
    m_isInitialized = true;
    if (commitType == FixupCommit) {
        QGroupBox *logChangeGroupBox = new QGroupBox(tr("Select Change"));
        QVBoxLayout *logChangeLayout = new QVBoxLayout;
        logChangeGroupBox->setLayout(logChangeLayout);
        m_logChangeWidget = new LogChangeWidget;
        m_logChangeWidget->init(repository, QString(), false);
        connect(m_logChangeWidget, SIGNAL(doubleClicked(QString)), this, SIGNAL(show(QString)));
        logChangeLayout->addWidget(m_logChangeWidget);
        insertTopWidget(logChangeGroupBox);
        m_gitSubmitPanelUi.editGroup->hide();
        hideDescription();
    }
    insertTopWidget(m_gitSubmitPanel);
}

void GitSubmitEditorWidget::refreshLog(const QString &repository)
{
    if (m_logChangeWidget)
        m_logChangeWidget->init(repository, QString(), false);
}

GitSubmitEditorPanelData GitSubmitEditorWidget::panelData() const
{
    GitSubmitEditorPanelData rc;
    rc.author = m_gitSubmitPanelUi.authorLineEdit->text();
    rc.email = m_gitSubmitPanelUi.emailLineEdit->text();
    rc.bypassHooks = m_gitSubmitPanelUi.bypassHooksCheckBox->isChecked();
    return rc;
}

void GitSubmitEditorWidget::setPanelData(const GitSubmitEditorPanelData &data)
{
    m_gitSubmitPanelUi.authorLineEdit->setText(data.author);
    m_gitSubmitPanelUi.emailLineEdit->setText(data.email);
    m_gitSubmitPanelUi.bypassHooksCheckBox->setChecked(data.bypassHooks);
    authorInformationChanged();
}

bool GitSubmitEditorWidget::canSubmit() const
{
    if (m_gitSubmitPanelUi.invalidAuthorLabel->isVisible()
        || m_gitSubmitPanelUi.invalidEmailLabel->isVisible()
        || m_hasUnmerged)
        return false;
    return SubmitEditorWidget::canSubmit();
}

QString GitSubmitEditorWidget::cleanupDescription(const QString &input) const
{
    // We need to manually purge out comment lines starting with
    // hash '#' since git does not do that when using -F.
    const QChar newLine = QLatin1Char('\n');
    const QChar hash = QLatin1Char('#');
    QString message = input;
    for (int pos = 0; pos < message.size(); ) {
        const int newLinePos = message.indexOf(newLine, pos);
        const int startOfNextLine = newLinePos == -1 ? message.size() : newLinePos + 1;
        if (message.at(pos) == hash)
            message.remove(pos, startOfNextLine - pos);
        else
            pos = startOfNextLine;
    }
    return message;

}

void GitSubmitEditorWidget::authorInformationChanged()
{
    bool bothEmpty = m_gitSubmitPanelUi.authorLineEdit->text().isEmpty() &&
            m_gitSubmitPanelUi.emailLineEdit->text().isEmpty();

    m_gitSubmitPanelUi.invalidAuthorLabel->
            setVisible(m_gitSubmitPanelUi.authorLineEdit->text().isEmpty() && !bothEmpty);
    m_gitSubmitPanelUi.invalidEmailLabel->
            setVisible(!emailIsValid() && !bothEmpty);

    updateSubmitAction();
}

bool GitSubmitEditorWidget::emailIsValid() const
{
    int pos = m_gitSubmitPanelUi.emailLineEdit->cursorPosition();
    QString text = m_gitSubmitPanelUi.emailLineEdit->text();
    return m_emailValidator->validate(text, pos) == QValidator::Acceptable;
}

} // namespace Internal
} // namespace Git
