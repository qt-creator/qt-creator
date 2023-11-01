// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectintropage.h"

#include "fancylineedit.h"
#include "filenamevalidatinglineedit.h"
#include "fileutils.h"
#include "infolabel.h"
#include "layoutbuilder.h"
#include "pathchooser.h"
#include "utilstr.h"
#include "wizard.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>

/*!
    \class Utils::ProjectIntroPage
    \inmodule QtCreator

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
    bool m_complete = false;
    QRegularExpressionValidator m_projectNameValidator;
    QString m_projectNameValidatorUserMessage;
    bool m_forceSubProject = false;
    FilePaths m_projectDirectories;

    QLabel *m_descriptionLabel;
    FancyLineEdit *m_nameLineEdit;
    PathChooser *m_pathChooser;
    QCheckBox *m_projectsDirectoryCheckBox;
    QLabel *m_projectLabel;
    QComboBox *m_projectComboBox;
    InfoLabel *m_stateLabel;
};

ProjectIntroPage::ProjectIntroPage(QWidget *parent) :
    WizardPage(parent),
    d(new ProjectIntroPagePrivate)
{
    setTitle(Tr::tr("Introduction and Project Location"));

    d->m_descriptionLabel = new QLabel(this);
    d->m_descriptionLabel->setWordWrap(true);

    auto frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Raised);

    d->m_nameLineEdit = new Utils::FancyLineEdit(frame);

    d->m_pathChooser = new Utils::PathChooser(frame);
    d->m_pathChooser->setObjectName("baseFolder"); // used by Squish
    d->m_pathChooser->setExpectedKind(PathChooser::Directory);
    d->m_pathChooser->setDisabled(d->m_forceSubProject);

    d->m_projectsDirectoryCheckBox = new QCheckBox(Tr::tr("Use as default project location"));
    d->m_projectsDirectoryCheckBox->setObjectName("projectsDirectoryCheckBox");
    d->m_projectsDirectoryCheckBox->setDisabled(d->m_forceSubProject);

    d->m_projectComboBox = new QComboBox;
    d->m_projectComboBox->setVisible(d->m_forceSubProject);

    d->m_stateLabel = new Utils::InfoLabel(this);
    d->m_stateLabel->setObjectName("stateLabel");
    d->m_stateLabel->setWordWrap(true);
    d->m_stateLabel->setFilled(true);
    hideStatusLabel();

    d->m_nameLineEdit->setPlaceholderText(Tr::tr("Enter project name"));
    d->m_nameLineEdit->setObjectName("nameLineEdit");
    d->m_nameLineEdit->setFocus();
    d->m_nameLineEdit->setValidationFunction([this](FancyLineEdit *edit, QString *errorString) {
        return validateProjectName(edit->text(), errorString);
    });

    d->m_projectLabel = new QLabel("Project:");
    d->m_projectLabel->setVisible(d->m_forceSubProject);

    using namespace Layouting;

    Form {
        Tr::tr("Name:"), d->m_nameLineEdit, br,
        d->m_projectLabel, d->m_projectComboBox, br,
        Column { Space(12) }, br,
        Tr::tr("Create in:"), d->m_pathChooser, br,
        Span(2, d->m_projectsDirectoryCheckBox)
    }.attachTo(frame);

    Column {
        d->m_descriptionLabel,
        st,
        frame,
        d->m_stateLabel
    }.attachTo(this);

    connect(d->m_pathChooser, &PathChooser::textChanged,
            this, &ProjectIntroPage::slotChanged);
    connect(d->m_nameLineEdit, &QLineEdit::textChanged,
            this, &ProjectIntroPage::slotChanged);
    connect(d->m_pathChooser, &PathChooser::validChanged,
            this, &ProjectIntroPage::slotChanged);
    connect(d->m_pathChooser, &PathChooser::returnPressed,
            this, &ProjectIntroPage::slotActivated);
    connect(d->m_nameLineEdit, &FancyLineEdit::validReturnPressed,
            this, &ProjectIntroPage::slotActivated);
    connect(d->m_projectComboBox, &QComboBox::currentIndexChanged,
            this, &ProjectIntroPage::slotChanged);

    setProperty(SHORT_TITLE_PROPERTY, Tr::tr("Location"));
    registerFieldWithName(QLatin1String("Path"), d->m_pathChooser, "path", SIGNAL(textChanged(QString)));
    registerFieldWithName(QLatin1String("ProjectName"), d->m_nameLineEdit);
}

ProjectIntroPage::~ProjectIntroPage()
{
    delete d;
}

QString ProjectIntroPage::projectName() const
{
    return d->m_nameLineEdit->text();
}

FilePath ProjectIntroPage::filePath() const
{
    return d->m_pathChooser->filePath();
}

void ProjectIntroPage::setFilePath(const FilePath &path)
{
    d->m_pathChooser->setFilePath(path);
}

void ProjectIntroPage::setProjectNameRegularExpression(const QRegularExpression &regEx, const QString &userErrorMessage)
{
    Q_ASSERT_X(regEx.isValid(), Q_FUNC_INFO, qPrintable(regEx.errorString()));
    d->m_projectNameValidator.setRegularExpression(regEx);
    d->m_projectNameValidatorUserMessage = userErrorMessage;
}

void ProjectIntroPage::setProjectName(const QString &name)
{
    d->m_nameLineEdit->setText(name);
    d->m_nameLineEdit->selectAll();
}

QString ProjectIntroPage::description() const
{
    return d->m_descriptionLabel->text();
}

void ProjectIntroPage::setDescription(const QString &description)
{
    d->m_descriptionLabel->setText(description);
}

bool ProjectIntroPage::isComplete() const
{
    return d->m_complete;
}

bool ProjectIntroPage::validate()
{
    if (d->m_forceSubProject) {
        int index = d->m_projectComboBox->currentIndex();
        if (index == 0)
            return false;
        d->m_pathChooser->setFilePath(d->m_projectDirectories.at(index));
    }
    // Validate and display status
    if (!d->m_pathChooser->isValid()) {
        displayStatusMessage(InfoLabel::Error, d->m_pathChooser->errorMessage());
        return false;
    }

    // Name valid?
    switch (d->m_nameLineEdit->state()) {
    case FancyLineEdit::Invalid:
        displayStatusMessage(InfoLabel::Error, d->m_nameLineEdit->errorMessage());
        return false;
    case FancyLineEdit::DisplayingPlaceholderText:
        displayStatusMessage(InfoLabel::Error, Tr::tr("Name is empty."));
        return false;
    case FancyLineEdit::Valid:
        break;
    }

    // Check existence of the directory
    const FilePath projectDir =
        filePath().pathAppended(QDir::fromNativeSeparators(d->m_nameLineEdit->text()));

    if (!projectDir.exists()) { // All happy
        if (!d->m_pathChooser->filePath().exists()) {
            displayStatusMessage(InfoLabel::Information, Tr::tr("Directory \"%1\" will be created.")
                                 .arg(d->m_pathChooser->filePath().toUserOutput()));
        } else {
            hideStatusLabel();
        }
        return true;
    }

    if (projectDir.isDir()) {
        displayStatusMessage(InfoLabel::Warning, Tr::tr("The project already exists."));
        return true;
    }
    // Not a directory, but something else, likely causing directory creation to fail
    displayStatusMessage(InfoLabel::Error, Tr::tr("A file with that name already exists."));
    return false;
}

void ProjectIntroPage::fieldsUpdated()
{
    slotChanged();
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
    d->m_projectLabel->setVisible(d->m_forceSubProject);
    d->m_projectComboBox->setVisible(d->m_forceSubProject);
    d->m_pathChooser->setDisabled(d->m_forceSubProject);
    d->m_projectsDirectoryCheckBox->setDisabled(d->m_forceSubProject);
}

void ProjectIntroPage::setProjectList(const QStringList &projectList)
{
    d->m_projectComboBox->clear();
    d->m_projectComboBox->addItems(projectList);
}

void ProjectIntroPage::setProjectDirectories(const FilePaths &directoryList)
{
    d->m_projectDirectories = directoryList;
}

int ProjectIntroPage::projectIndex() const
{
    return d->m_projectComboBox->currentIndex();
}

bool ProjectIntroPage::validateProjectName(const QString &name, QString *errorMessage)
{
    int pos = -1;
    // if we have a pattern it was set
    if (!d->m_projectNameValidator.regularExpression().pattern().isEmpty()) {
        if (name.isEmpty()) {
            if (errorMessage)
                *errorMessage = Tr::tr("Name is empty.");
            return false;
        }
        // pos is set by reference
        QString tmp = name;
        QValidator::State validatorState = d->m_projectNameValidator.validate(tmp, pos);

        // if pos is set by validate it is cought at the bottom where it shows
        // a more detailed error message
        if (validatorState != QValidator::Acceptable && (pos == -1 || pos >= name.size())) {
            if (errorMessage) {
                if (d->m_projectNameValidatorUserMessage.isEmpty())
                    *errorMessage = Tr::tr("Project name is invalid.");
                else
                    *errorMessage = d->m_projectNameValidatorUserMessage;
            }
            return false;
        }
    } else { // no validator means usually a qmake project
        // Validation is file name + checking for dots
        if (!FileNameValidatingLineEdit::validateFileName(name, false, errorMessage))
            return false;
        if (name.contains(QLatin1Char('.'))) {
            if (errorMessage)
                *errorMessage = Tr::tr("Invalid character \".\".");
            return false;
        }
        pos = FileUtils::indexOfQmakeUnfriendly(name);
    }
    if (pos >= 0) {
        if (errorMessage)
            *errorMessage = Tr::tr("Invalid character \"%1\" found.").arg(name.at(pos));
        return false;
    }
    return true;
}

void ProjectIntroPage::displayStatusMessage(InfoLabel::InfoType t, const QString &s)
{
    d->m_stateLabel->setType(t);
    d->m_stateLabel->setText(s);

    emit statusMessageChanged(t, s);
}

void ProjectIntroPage::hideStatusLabel()
{
    displayStatusMessage(InfoLabel::None, {});
}

bool ProjectIntroPage::useAsDefaultPath() const
{
    return d->m_projectsDirectoryCheckBox->isChecked();
}

void ProjectIntroPage::setUseAsDefaultPath(bool u)
{
    d->m_projectsDirectoryCheckBox->setChecked(u);
}

} // namespace Utils
