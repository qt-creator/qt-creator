/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "projectintropage.h"
#include "filewizardpage.h"
#include "ui_projectintropage.h"

#include <QtGui/QMessageBox>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>


/*!
    \class Utils::ProjectIntroPage

    \brief Standard wizard page for a project, letting the user choose name
    and path.

    Looks similar to FileWizardPage, but provides additional
    functionality:
    \list
    \o Description label at the top for displaying introductory text
    \o It does on the fly validation (connected to changed()) and displays
       warnings/errors in a status label at the bottom (the page is complete
       when fully validated, validatePage() is thus not implemented).
    \endlist

    Note: Careful when changing projectintropage.ui. It must have main
    geometry cleared and QLayout::SetMinimumSize constraint on the main
    layout, otherwise, QWizard will squeeze it due to its strange expanding
    hacks.
*/

namespace Utils {

struct ProjectIntroPagePrivate
{
    ProjectIntroPagePrivate();
    Ui::ProjectIntroPage m_ui;
    bool m_complete;
    // Status label style sheets
    const QString m_errorStyleSheet;
    const QString m_warningStyleSheet;
    const QString m_hintStyleSheet;
};

ProjectIntroPagePrivate::  ProjectIntroPagePrivate() :
    m_complete(false),
    m_errorStyleSheet(QLatin1String("background : red;")),
    m_warningStyleSheet(QLatin1String("background : yellow;")),
    m_hintStyleSheet()
{
}

ProjectIntroPage::ProjectIntroPage(QWidget *parent) :
    QWizardPage(parent),
    m_d(new ProjectIntroPagePrivate)
{
    m_d->m_ui.setupUi(this);
    hideStatusLabel();
    m_d->m_ui.nameLineEdit->setInitialText(tr("<Enter_Name>"));
    m_d->m_ui.nameLineEdit->setFocus();
    connect(m_d->m_ui.pathChooser, SIGNAL(changed(QString)), this, SLOT(slotChanged()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(m_d->m_ui.pathChooser, SIGNAL(validChanged()), this, SLOT(slotChanged()));
    connect(m_d->m_ui.pathChooser, SIGNAL(returnPressed()), this, SLOT(slotActivated()));
    connect(m_d->m_ui.nameLineEdit, SIGNAL(validReturnPressed()), this, SLOT(slotActivated()));
}

void ProjectIntroPage::insertControl(int row, QWidget *label, QWidget *control)
{
    m_d->m_ui.formLayout->insertRow(row, label, control);
}

ProjectIntroPage::~ProjectIntroPage()
{
    delete m_d;
}

QString ProjectIntroPage::projectName() const
{
    return m_d->m_ui.nameLineEdit->text();
}

QString ProjectIntroPage::path() const
{
    return m_d->m_ui.pathChooser->path();
}

void ProjectIntroPage::setPath(const QString &path)
{
    m_d->m_ui.pathChooser->setPath(path);
}

void ProjectIntroPage::setProjectName(const QString &name)
{
    m_d->m_ui.nameLineEdit->setText(name);
    m_d->m_ui.nameLineEdit->selectAll();
}

QString ProjectIntroPage::description() const
{
    return m_d->m_ui.descriptionLabel->text();
}

void ProjectIntroPage::setDescription(const QString &description)
{
    m_d->m_ui.descriptionLabel->setText(description);
}

void ProjectIntroPage::changeEvent(QEvent *e)
{
    QWizardPage::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_d->m_ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

bool ProjectIntroPage::isComplete() const
{
    return m_d->m_complete;
}

bool ProjectIntroPage::validate()
{
    // Validate and display status
    if (!m_d->m_ui.pathChooser->isValid()) {
        displayStatusMessage(Error, m_d->m_ui.pathChooser->errorMessage());
        return false;
    }

    // Name valid? Ignore 'DisplayingInitialText' state.
    bool nameValid = false;
    switch (m_d->m_ui.nameLineEdit->state()) {
    case BaseValidatingLineEdit::Invalid:
        displayStatusMessage(Error, m_d->m_ui.nameLineEdit->errorMessage());
        return false;
    case BaseValidatingLineEdit::DisplayingInitialText:
        break;
    case BaseValidatingLineEdit::Valid:
        nameValid = true;
        break;
    }

    // Check existence of the directory
    QString projectDir = path();
    projectDir += QDir::separator();
    projectDir += m_d->m_ui.nameLineEdit->text();
    const QFileInfo projectDirFile(projectDir);
    if (!projectDirFile.exists()) { // All happy
        hideStatusLabel();
        return nameValid;
    }

    if (projectDirFile.isDir()) {
        displayStatusMessage(Warning, tr("The project already exists."));
        return nameValid;;
    }
    // Not a directory, but something else, likely causing directory creation to fail
    displayStatusMessage(Error, tr("A file with that name already exists."));
    return false;
}

void ProjectIntroPage::slotChanged()
{
    const bool newComplete = validate();
    if (newComplete != m_d->m_complete) {
        m_d->m_complete = newComplete;
        emit completeChanged();
    }
}

void ProjectIntroPage::slotActivated()
{
    if (m_d->m_complete)
        emit activated();
}

bool ProjectIntroPage::validateProjectDirectory(const QString &name, QString *errorMessage)
{
    return ProjectNameValidatingLineEdit::validateProjectName(name, errorMessage);
}

void ProjectIntroPage::displayStatusMessage(StatusLabelMode m, const QString &s)
{
    switch (m) {
    case Error:
        m_d->m_ui.stateLabel->setStyleSheet(m_d->m_errorStyleSheet);
        break;
    case Warning:
        m_d->m_ui.stateLabel->setStyleSheet(m_d->m_warningStyleSheet);
        break;
    case Hint:
        m_d->m_ui.stateLabel->setStyleSheet(m_d->m_hintStyleSheet);
        break;
    }
    m_d->m_ui.stateLabel->setText(s);
}

void ProjectIntroPage::hideStatusLabel()
{
    displayStatusMessage(Hint, QString());
}

bool ProjectIntroPage::useAsDefaultPath() const
{
    return m_d->m_ui.projectsDirectoryCheckBox->isChecked();
}

void ProjectIntroPage::setUseAsDefaultPath(bool u)
{
    m_d->m_ui.projectsDirectoryCheckBox->setChecked(u);
}

} // namespace Utils
