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

#include "projectintropage.h"
#include "ui_projectintropage.h"

#include "filenamevalidatinglineedit.h"
#include "wizard.h"

#include <QDir>

/*!
    \class Utils::ProjectIntroPage

    \brief The ProjectIntroPage class is the standard wizard page for a project,
    letting the user choose its name
    and path.

    Looks similar to FileWizardPage, but provides additional
    functionality:
    \list
    \li Contains a description label at the top for displaying introductory text.
    \li Does on the fly validation (connected to changed()) and displays
       warnings and errors in a status label at the bottom (the page is complete
       when fully validated, validatePage() is thus not implemented).
    \endlist

    \note Careful when changing projectintropage.ui. It must have main
    geometry cleared and QLayout::SetMinimumSize constraint on the main
    layout, otherwise, QWizard will squeeze it due to its strange expanding
    hacks.
*/

namespace Utils {

class ProjectIntroPagePrivate
{
public:
    ProjectIntroPagePrivate();
    Ui::ProjectIntroPage m_ui;
    bool m_complete;
    QRegularExpressionValidator m_projectNameValidator;
    // Status label style sheets
    const QString m_errorStyleSheet;
    const QString m_warningStyleSheet;
    const QString m_hintStyleSheet;
    bool m_forceSubProject;
    QStringList m_projectDirectories;
};

ProjectIntroPagePrivate::  ProjectIntroPagePrivate() :
    m_complete(false),
    m_errorStyleSheet(QLatin1String("background : red;")),
    m_warningStyleSheet(QLatin1String("background : yellow;")),
    m_hintStyleSheet(),
    m_forceSubProject(false)
{
}

ProjectIntroPage::ProjectIntroPage(QWidget *parent) :
    WizardPage(parent),
    d(new ProjectIntroPagePrivate)
{
    d->m_ui.setupUi(this);
    hideStatusLabel();
    d->m_ui.nameLineEdit->setInitialText(tr("<Enter_Name>"));
    d->m_ui.nameLineEdit->setFocus();
    d->m_ui.nameLineEdit->setValidationFunction([this](FancyLineEdit *edit, QString *errorString) {
        return validateProjectName(edit->text(), errorString);
    });
    d->m_ui.projectLabel->setVisible(d->m_forceSubProject);
    d->m_ui.projectComboBox->setVisible(d->m_forceSubProject);
    d->m_ui.pathChooser->setDisabled(d->m_forceSubProject);
    d->m_ui.projectsDirectoryCheckBox->setDisabled(d->m_forceSubProject);
    connect(d->m_ui.pathChooser, &PathChooser::pathChanged, this, &ProjectIntroPage::slotChanged);
    connect(d->m_ui.nameLineEdit, &QLineEdit::textChanged,
            this, &ProjectIntroPage::slotChanged);
    connect(d->m_ui.pathChooser, &PathChooser::validChanged,
            this, &ProjectIntroPage::slotChanged);
    connect(d->m_ui.pathChooser, &PathChooser::returnPressed,
            this, &ProjectIntroPage::slotActivated);
    connect(d->m_ui.nameLineEdit, &FancyLineEdit::validReturnPressed,
            this, &ProjectIntroPage::slotActivated);
    connect(d->m_ui.projectComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ProjectIntroPage::slotChanged);

    setProperty(SHORT_TITLE_PROPERTY, tr("Location"));
    registerFieldWithName(QLatin1String("Path"), d->m_ui.pathChooser, "path", SIGNAL(pathChanged(QString)));
    registerFieldWithName(QLatin1String("ProjectName"), d->m_ui.nameLineEdit);
}

void ProjectIntroPage::insertControl(int row, QWidget *label, QWidget *control)
{
    d->m_ui.formLayout->insertRow(row, label, control);
}

ProjectIntroPage::~ProjectIntroPage()
{
    delete d;
}

QString ProjectIntroPage::projectName() const
{
    return d->m_ui.nameLineEdit->text();
}

QString ProjectIntroPage::path() const
{
    return d->m_ui.pathChooser->path();
}

void ProjectIntroPage::setPath(const QString &path)
{
    d->m_ui.pathChooser->setPath(path);
}

void ProjectIntroPage::setProjectNameRegularExpression(const QRegularExpression &regEx)
{
    Q_ASSERT_X(regEx.isValid(), Q_FUNC_INFO, qPrintable(regEx.errorString()));
    d->m_projectNameValidator.setRegularExpression(regEx);
}

void ProjectIntroPage::setProjectName(const QString &name)
{
    d->m_ui.nameLineEdit->setText(name);
    d->m_ui.nameLineEdit->selectAll();
}

QString ProjectIntroPage::description() const
{
    return d->m_ui.descriptionLabel->text();
}

void ProjectIntroPage::setDescription(const QString &description)
{
    d->m_ui.descriptionLabel->setText(description);
}

bool ProjectIntroPage::isComplete() const
{
    return d->m_complete;
}

bool ProjectIntroPage::validate()
{
    if (d->m_forceSubProject) {
        int index = d->m_ui.projectComboBox->currentIndex();
        if (index == 0)
            return false;
        d->m_ui.pathChooser->setPath(d->m_projectDirectories.at(index));
    }
    // Validate and display status
    if (!d->m_ui.pathChooser->isValid()) {
        displayStatusMessage(Error, d->m_ui.pathChooser->errorMessage());
        return false;
    }

    // Name valid? Ignore 'DisplayingInitialText' state.
    bool nameValid = false;
    switch (d->m_ui.nameLineEdit->state()) {
    case FancyLineEdit::Invalid:
        displayStatusMessage(Error, d->m_ui.nameLineEdit->errorMessage());
        return false;
    case FancyLineEdit::DisplayingInitialText:
        break;
    case FancyLineEdit::Valid:
        nameValid = true;
        break;
    }

    // Check existence of the directory
    const QFileInfo projectDirFile(path() + QLatin1Char('/')
                                   + QDir::fromNativeSeparators(d->m_ui.nameLineEdit->text()));
    if (!projectDirFile.exists()) { // All happy
        hideStatusLabel();
        return nameValid;
    }

    if (projectDirFile.isDir()) {
        displayStatusMessage(Warning, tr("The project already exists."));
        return nameValid;
    }
    // Not a directory, but something else, likely causing directory creation to fail
    displayStatusMessage(Error, tr("A file with that name already exists."));
    return false;
}

void ProjectIntroPage::slotChanged()
{
    const bool newComplete = validate();
    if (newComplete != d->m_complete) {
        d->m_complete = newComplete;
        emit completeChanged();
    }
}

void ProjectIntroPage::slotActivated()
{
    if (d->m_complete)
        emit activated();
}

bool ProjectIntroPage::forceSubProject() const
{
    return d->m_forceSubProject;
}

void ProjectIntroPage::setForceSubProject(bool force)
{
    d->m_forceSubProject = force;
    d->m_ui.projectLabel->setVisible(d->m_forceSubProject);
    d->m_ui.projectComboBox->setVisible(d->m_forceSubProject);
    d->m_ui.pathChooser->setDisabled(d->m_forceSubProject);
    d->m_ui.projectsDirectoryCheckBox->setDisabled(d->m_forceSubProject);
}

void ProjectIntroPage::setProjectList(const QStringList &projectList)
{
    d->m_ui.projectComboBox->clear();
    d->m_ui.projectComboBox->addItems(projectList);
}

void ProjectIntroPage::setProjectDirectories(const QStringList &directoryList)
{
    d->m_projectDirectories = directoryList;
}

int ProjectIntroPage::projectIndex() const
{
    return d->m_ui.projectComboBox->currentIndex();
}

bool ProjectIntroPage::validateProjectName(const QString &name, QString *errorMessage)
{
    int pos = -1;
    // if we have a pattern it was set
    if (!d->m_projectNameValidator.regularExpression().pattern().isEmpty()) {
        if (name.isEmpty()) {
            if (errorMessage)
                *errorMessage = tr("Name is empty.");
            return false;
        }
        // pos is set by reference
        QString tmp = name;
        QValidator::State validatorState = d->m_projectNameValidator.validate(tmp, pos);

        // if pos is set by validate it is cought at the bottom where it shows
        // a more detailed error message
        if (validatorState != QValidator::Acceptable && (pos == -1 || pos >= name.count())) {
            if (errorMessage) {
                *errorMessage = tr("Name does not match \"%1\".").arg(
                    d->m_projectNameValidator.regularExpression().pattern());
            }
            return false;
        }
    } else { // no validator means usually a qmake project
        // Validation is file name + checking for dots
        if (!FileNameValidatingLineEdit::validateFileName(name, false, errorMessage))
            return false;
        if (name.contains(QLatin1Char('.'))) {
            if (errorMessage)
                *errorMessage = tr("Invalid character \".\".");
            return false;
        }
        pos = FileUtils::indexOfQmakeUnfriendly(name);
    }
    if (pos >= 0) {
        if (errorMessage)
            *errorMessage = tr("Invalid character \"%1\" found.").arg(name.at(pos));
        return false;
    }
    return true;
}

void ProjectIntroPage::displayStatusMessage(StatusLabelMode m, const QString &s)
{
    switch (m) {
    case Error:
        d->m_ui.stateLabel->setStyleSheet(d->m_errorStyleSheet);
        break;
    case Warning:
        d->m_ui.stateLabel->setStyleSheet(d->m_warningStyleSheet);
        break;
    case Hint:
        d->m_ui.stateLabel->setStyleSheet(d->m_hintStyleSheet);
        break;
    }
    d->m_ui.stateLabel->setText(s);
}

void ProjectIntroPage::hideStatusLabel()
{
    displayStatusMessage(Hint, QString());
}

bool ProjectIntroPage::useAsDefaultPath() const
{
    return d->m_ui.projectsDirectoryCheckBox->isChecked();
}

void ProjectIntroPage::setUseAsDefaultPath(bool u)
{
    d->m_ui.projectsDirectoryCheckBox->setChecked(u);
}

} // namespace Utils
